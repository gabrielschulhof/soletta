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
#include <nan.h>

#include "bridge2.h"

using namespace v8;

static void theBridgeIsOut(const Nan::WeakCallbackInfo<BridgeNode> &data);

struct BridgeNodeNotEqual {
	bool operator()(const BridgeNode *left, const BridgeNode *right) const;
};

static std::set<BridgeNode *, BridgeNodeNotEqual > lookup;

bool BridgeNode::operator==(const BridgeNode & other) const {
	return (((*local) == (*(other.local))) && (release == other.release));
}

BridgeNode::BridgeNode(Local<Object> _local, void (*_release)(void *)):
	local(new Nan::Persistent<Object>(_local)), release(_release),
	isNew(true), callRelease(true) {
	printf("Created persistent %p\n", local);
}

BridgeNode::~BridgeNode() {
	printf("Deleted persistent %p\n", local);
	local->ClearWeak();
	local->Reset();
	delete local;

	if (isNew) {
		return;
	}

	if (release && callRelease) {
		release(this);
	}

	BridgeNode *theAdjacent, *backReference;
	for (std::list<BridgeNode *>::iterator
			iter = adjacents.begin();
			iter != adjacents.end();
			iter++) {
		theAdjacent = *iter;
		if (theAdjacent != this) {
			for (std::list<BridgeNode *>::iterator
					backIter = theAdjacent->adjacents.begin();
					backIter != theAdjacent->adjacents.end();
					backIter++) {
				backReference = *backIter;
				if (operator==(*backReference)) {
					backIter = theAdjacent->adjacents.erase(backIter);
				}
			}
			if (theAdjacent->adjacents.empty()) {
				delete theAdjacent;
			}
		}
	}

	lookup.erase(this);
}

void BridgeNode::Delete(BridgeNode *theBridge, bool callRelease = true) {
	theBridge->callRelease = callRelease;
	delete theBridge;
}

static void theBridgeIsOut(const Nan::WeakCallbackInfo<BridgeNode> &data) {
	printf("theBridgeIsOut: persistent: %p\n",
		((BridgeNode *)data.GetParameter())->local);
	delete data.GetParameter();
}

bool BridgeNodeNotEqual::operator()(
		const BridgeNode *left,
		const BridgeNode *right) const {
	return !((*left) == (*right));
}

BridgeNode *BridgeNode::Acquire() {
	std::set<BridgeNode *, BridgeNodeNotEqual>::iterator
		iter = lookup.find(this);
	return ((iter == lookup.end()) ?
		new BridgeNode(Nan::New<Object>(*local), release) : (*iter));
}

void BridgeNode::Release(BridgeNode *theBridge) {
	if (theBridge && theBridge->isNew) {
		delete theBridge;
	}
}

void BridgeNode::AddAdjacent(BridgeNode *theOther) {
	if (isNew) {
		local->SetWeak(this, theBridgeIsOut,
			Nan::WeakCallbackType::kParameter);
		printf("Weakened persistent %p\n", local);
		isNew = false;
		lookup.insert(this);
	}
	adjacents.push_back(theOther);

	if (operator==(*theOther)) {
		BridgeNode::Release(theOther);
	} else {
		theOther->AddAdjacent(this);
	}
}

void BridgeNode::AddAdjacent(Local<Object> value,
		void (*releaseValue)(void *)) {
	AddAdjacent(BridgeNode(value, releaseValue).Acquire());
}

Local<Array> BridgeNode::Call(int argumentCount, Local<Value> arguments[]) {
	std::list<Nan::Persistent<Object> *> callbacks;
	for (std::list<BridgeNode *>::iterator
			iter = adjacents.begin();
			iter != adjacents.end();
			iter++) {
		Nan::Persistent<Object> *adjacent = (*iter)->local;
		if (Nan::New<Object>(*adjacent)->IsFunction()) {
			callbacks.push_back(adjacent);
		}
	}
	Local<Array> returnValues = Nan::New<Array>(callbacks.size());
	int index = 0;
	for (std::list<Nan::Persistent<Object> *>::iterator
			iter = callbacks.begin();
			iter != callbacks.end();
			iter++, index++) {
		Nan::Set(returnValues, index,
			Nan::Call(Local<Function>::Cast(Nan::New<Object>(**iter)),
				Nan::New<Object>(), argumentCount,
					arguments).ToLocalChecked());
	}
	return returnValues;
}

#ifdef DBG_BRIDGE
#include <stdio.h>
static void dump_bridge_node(BridgeNode *theBridge) {
	printf("%p: %s\n", theBridge, *String::Utf8Value(
		Nan::Get(Nan::New<Object>(*(theBridge->local)),
			Nan::New("_bridge_debug").ToLocalChecked())
			.ToLocalChecked()->ToString()));
	for (std::list<BridgeNode *>::iterator
			iter = theBridge->adjacents.begin();
			iter != theBridge->adjacents.end();
			iter++) {
		printf("->%s\n", *String::Utf8Value(
			Nan::Get(Nan::New<Object>(*(*iter)->local),
				Nan::New("_bridge_debug").ToLocalChecked())
				.ToLocalChecked()->ToString()));
	}
}

void async_bridge_dump(const char *prefix) {
	printf("%s\n", prefix);
	for (std::set<BridgeNode *, BridgeNodeNotEqual>::iterator
			iter = lookup.begin();
			iter != lookup.end();
			iter++) {
		dump_bridge_node(*iter);
	}
}
#endif /* DBG_BRIDGE */
