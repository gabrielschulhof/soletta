/*
 * This file is part of the Soletta Project
 *
 * Copyright (C) 2015 Intel Corporation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in
 *     the documentation and/or other materials provided with the
 *     distribution.
 *   * Neither the name of Intel Corporation nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <set>
#include <list>
#include "bridge.h"
#include "hijack.h"

/*
 * Setup and assumptions:
 * 1. The C callback data is always a Nan::Callback*
 * 2. If a key is garbage-collected, the corresponding callbacks leak.
 *
 *    Justification: Consider:
 *      var w = function() {};
 *      var x = setInterval( w, 1000 );
 *      x = undefined;
 *    Now, there is no way to clearInterval(x).
 *
 *    Note: This makes it possible to issue a SOL_WRN() when Nan::Callback*
 *    lists become unreachable by adding a weak callback to the persistents we
 *    keep.
 * 3. The AsyncBridge is a one-to-many map of persistent handles to lists of
 *    callbacks.
 * X. Persistent Object handles are weak, and the keys they represent are
 *    removed from the map when they are garbage-collected.
 * 5. The keys representing the persistent handles are removed from the map
 *    when they no longer refer to any lists of callbacks.
 * 6. A list of callbacks is referred to by a unique combination of keys.
 *
 * Use cases:
 * 1. Make the same call twice and later remove:
 *    bridge.add( [ key1, key2 ], theFunction);
 *    bridge.add( [ key1, key2 ], theFunction);
 *    ...
 *    bridge.remove( [ key1, key2 ], theFunction);
 *    The remove() call removes the first instance of theFunction.
 */

using namespace v8;

typedef std::set<BridgeNode *> BridgeNodes;

class KeyNode {
public:
	KeyNode(Local<Value> _key);
	~KeyNode();
	Nan::Persistent<Value> *key;
	BridgeNodes references;
};

struct KeysNotEqual {
	bool operator()(const KeyNode *left, const KeyNode *right) const;
};

typedef std::set<KeyNode *, KeysNotEqual> Keys;
typedef std::list<Nan::Callback *> Callbacks;

class BridgeNode {
public:
	BridgeNode(Keys & _backReferences, Nan::Callback *callback);
	~BridgeNode();
	Nan::Callback *findCallback(Local<Function> jsCallback);
	void removeCallback(Nan::Callback *callback);
	Callbacks callbacks;
	Keys backReferences;
};

static Keys lookup;

bool KeysNotEqual::operator()(const KeyNode *left,
	const KeyNode *right) const {
	return (!Nan::Equals(Nan::New<Value>(*(left->key)),
		Nan::New<Value>(*(right->key))).FromJust());
}

KeyNode::KeyNode(Local<Value> _key): key(new Nan::Persistent<Value>(_key)) {}

KeyNode::~KeyNode() {
	if (references.size() > 0) {
		lookup.erase(this);
		BridgeNodes toRemove;
		for (BridgeNodes::iterator iter = references.begin();
				iter != references.end();) {
			(*iter)->backReferences.erase(this);
			if ((*iter)->backReferences.size() == 0) {
				toRemove.insert(*(iter));
				references.erase(iter++);
			} else {
				++iter;
			}
		}
		for (BridgeNodes::iterator iter = toRemove.begin();
			iter != toRemove.end();
			iter++) {
			delete *iter;
		}
	}
	key->Reset();
	delete key;
}

BridgeNode::BridgeNode(Keys & _backReferences,
	Nan::Callback *callback): backReferences(_backReferences) {
	callbacks.push_back(callback);
	for (Keys::iterator iter = backReferences.begin();
			iter != backReferences.end();
			iter++) {
		(*iter)->references.insert(this);
	}
}

BridgeNode::~BridgeNode() {
	Keys keysToErase;
	for (Keys::iterator iter = backReferences.begin();
			iter != backReferences.end();) {
		(*iter)->references.erase(this);
		if ((*iter)->references.size() == 0) {
			keysToErase.insert(*iter);
			backReferences.erase(iter++);
		} else {
			++iter;
		}
	}
	for (Keys::iterator iter = keysToErase.begin();
			iter != keysToErase.end();
			iter++) {
		delete *iter;
	}
}

Nan::Callback *BridgeNode::findCallback(Local<Function> jsCallback) {
	Nan::Callback compareValue(jsCallback);

	for (Callbacks::iterator iter = callbacks.begin(); iter != callbacks.end();
			iter++) {
		if ((**iter) == compareValue) {
			return *iter;
		}
	}

	return 0;
}

void BridgeNode::removeCallback(Nan::Callback *callback) {
	for (Callbacks::iterator iter = callbacks.begin(); iter != callbacks.end();
			iter++) {
		if ((**iter) == *callback) {
			callbacks.erase(iter);
			break;
		}
	}
}

static bool initKeyNodes(int keyCount, Local<Value>keys[], Keys & keyNodes,
	bool insert) {

	bool needsNewBridge = false;
	int index;
	KeyNode *oneKey;

	for (index = 0; index < keyCount; index++) {
		oneKey = new KeyNode(keys[index]);
		Keys::iterator iter = lookup.find(oneKey);
		if (iter == lookup.end()) {
			needsNewBridge = true;
			if (insert) {
				keyNodes.insert(oneKey);
				lookup.insert(oneKey);
			} else {
				delete oneKey;
				break;
			}
		} else {
			keyNodes.insert(*iter);
			delete oneKey;
		}
	}
	return needsNewBridge;
}

static BridgeNode *findBridge(Keys & keyNodes) {

	// It's enough to look through the bridges associated with the first key,
	// because if there's a bridge referring back to all the keys in keyNodes,
	// we will find a reference to that same bridge in all the other members of
	// keyNodes.
	KeyNode *oneKey = *(keyNodes.begin());
	for (BridgeNodes::iterator iter = oneKey->references.begin();
			iter != oneKey->references.end();
			iter++) {
		if ((*iter)->backReferences == keyNodes) {
			return (*iter);
		}
	}
	return 0;
}

void async_bridge_add(int keyCount, v8::Local<v8::Value> keys[],
	Nan::Callback *callback) {

	if (keyCount <= 0) {
		Nan::ThrowError("async_bridge_add: keyCount <= 0");
		return;
	}

	Keys keyNodes;

	// If no new keys were created, we look for an existing bridge
	if (!initKeyNodes(keyCount, keys, keyNodes, true)) {
		BridgeNode *theBridge = findBridge(keyNodes);
		if (theBridge) {
			theBridge->callbacks.push_back(callback);
			goto done;
		}
	}

	// If we don't find a bridge, we make a new one. We need not save the new
	// object, because it adds references to itself to all the keyNodes which,
	// in turn, are stored in the lookup.
	new BridgeNode(keyNodes, callback);
done:
	hijack_ref();
}

Nan::Callback *async_bridge_get(int keyCount, v8::Local<v8::Value> keys[],
	Local<Function> jsCallback, BridgeNode **bridgeNode) {

	if (keyCount <= 0) {
		Nan::ThrowError("async_bridge_get: keyCount <= 0");
		return 0;
	}

	Keys keyNodes;
	if (initKeyNodes(keyCount, keys, keyNodes, false)) {
		Nan::ThrowError("async_bridge_get: bridge not found (unknown keys)");
		return 0;
	}

	BridgeNode *theBridge = findBridge(keyNodes);
	if (!theBridge) {
		Nan::ThrowError("async_bridge_get: bridge node found");
		return 0;
	}

	Nan::Callback *theCallback = theBridge->findCallback(jsCallback);
	if (!theCallback) {
		Nan::ThrowError("async_bridge_get: callback not found");
		return 0;
	}

	*bridgeNode = theBridge;
	return theCallback;
}

void async_bridge_remove(BridgeNode *theBridge, Nan::Callback *callback) {
	theBridge->removeCallback(callback);
	if (theBridge->callbacks.size() == 0) {
		delete theBridge;
	}
	hijack_unref();
}
