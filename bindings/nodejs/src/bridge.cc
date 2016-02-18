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
 *    Now, there is no way to clearInterval(x), and a reference to w leaks.
 *
 *    Implementation: Add a weak callback to the key and remove it from the
 *    lookup. This is only possible for Object-typed persistents.
 * Use cases:
 * 1. Make the same call twice and later remove:
 *    bridge.add( [ key1, key2 ], theFunction);
 *    bridge.add( [ key1, key2 ], theFunction);
 *    ...
 *    bridge.remove( [ key1, key2 ], theFunction);
 *    The remove() call removes the first instance of theFunction.
 *
 * 3. The AsyncBridge is a one-to-many map of persistent handles to lists of
 *    callbacks.
 * 4. Persistent Object handles are weak, and the keys they represent are
 *    removed from the map when they are garbage-collected.
 * 5. The keys representing the persistent handles are removed from the map
 *    when they no longer refer to any lists of callbacks.
 * 6. A list of callbacks is referred to by a unique combination of keys.
 */

using namespace v8;

typedef struct _KeyNode KeyNode;

struct KeysNotEqual {
	bool operator()(const KeyNode *left, const KeyNode *right) const;
};

typedef std::set<KeyNode *, KeysNotEqual> Keys;
typedef std::list<Nan::Callback *> Callbacks;
typedef std::set<BridgeNode *> BridgeNodes;


struct _BridgeNode {
	_BridgeNode(Keys & _backReferences, Nan::Callback *callback);
	~_BridgeNode();
	Nan::Callback *findCallback(Local<Function> jsCallback);
	void removeCallback(Nan::Callback *callback);
	Callbacks callbacks;
	Keys backReferences;
};

struct _KeyNode {
	bool operator==(const KeyNode & other) const;
	_KeyNode(Local<Value> _key);
	~_KeyNode();
	Nan::Persistent<Value> *key;
	BridgeNodes references;
};

static Keys lookup;

bool KeysNotEqual::operator()(const KeyNode *left,
	const KeyNode *right) const {
	return (!((*left) == (*right)));
}

bool _KeyNode::operator==(const KeyNode & other) const {
	return Nan::Equals(Nan::New<Value>(*key),
		Nan::New<Value>(*(other.key))).FromJust();
}

_KeyNode::_KeyNode(Local<Value> _key): key(new Nan::Persistent<Value>(_key)) {}

_KeyNode::~_KeyNode() {
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

_BridgeNode::_BridgeNode(Keys & _backReferences,
	Nan::Callback *callback): backReferences(_backReferences) {
	callbacks.push_back(callback);
	for (Keys::iterator iter = backReferences.begin();
			iter != backReferences.end();
			iter++) {
		(*iter)->references.insert(this);
	}
}

_BridgeNode::~_BridgeNode() {
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

Nan::Callback *_BridgeNode::findCallback(Local<Function> jsCallback) {
	Nan::Callback compareValue(jsCallback);

	for (Callbacks::iterator iter = callbacks.begin(); iter != callbacks.end();
			iter++) {
		if ((**iter) == compareValue) {
			return *iter;
		}
	}

	return 0;
}

void _BridgeNode::removeCallback(Nan::Callback *callback) {
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

_BridgeCallback::_BridgeCallback(Nan::Callback *_callback,
	BridgeNode *_theBridge): callback(_callback), theBridge(_theBridge) {}

void async_bridge_add(int keyCount, v8::Local<v8::Value> keys[],
	Nan::Callback *callback) {

	if (keyCount <= 0) {
		Nan::ThrowError("async_bridge_add: keyCount <= 0");
		return;
	}

	Keys keyNodes;
	bool needsNewBridge = false;

	// Initialize keyNodes
	needsNewBridge = initKeyNodes(keyCount, keys, keyNodes, true);

	// If all the keys were already in the lookup, we need to find a bridge
	// to which they all refer. It's enough to check the bridge nodes of the
	// first key, because the bridge node will have references back to the
	// other keys we want.
	if (!needsNewBridge) {
		BridgeNode *theBridge = findBridge(keyNodes);
		if (theBridge) {
			theBridge->callbacks.push_back(callback);
		} else {
			needsNewBridge = true;
		}
	}

	// The easy case - insertion. We need not save the new object, because it
	// adds references to itself to all the keyNodes which, in turn, are stored
	// in the lookup.
	if (needsNewBridge) {
		new BridgeNode(keyNodes, callback);
	}

	hijack_ref();
}

BridgeCallback *async_bridge_get(int keyCount, v8::Local<v8::Value> keys[],
	Local<Function> jsCallback) {

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

	return new BridgeCallback(theCallback, theBridge);
}

void async_bridge_remove(BridgeCallback *bridgeCallback) {
	bridgeCallback->theBridge->removeCallback(bridgeCallback->callback);
	if (bridgeCallback->theBridge->callbacks.size() == 0) {
		delete bridgeCallback->theBridge;
	}
	hijack_unref();
}
