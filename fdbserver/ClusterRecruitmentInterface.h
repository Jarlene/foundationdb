/*
 * ClusterRecruitmentInterface.h
 *
 * This source file is part of the FoundationDB open source project
 *
 * Copyright 2013-2018 Apple Inc. and the FoundationDB project authors
 * 
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * 
 *     http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef FDBSERVER_CLUSTERRECRUITMENTINTERFACE_H
#define FDBSERVER_CLUSTERRECRUITMENTINTERFACE_H
#pragma once

#include "fdbclient/ClusterInterface.h"
#include "fdbclient/StorageServerInterface.h"
#include "fdbclient/MasterProxyInterface.h"
#include "DatabaseConfiguration.h"
#include "MasterInterface.h"
#include "TLogInterface.h"
#include "WorkerInterface.h"
#include "Knobs.h"

// This interface and its serialization depend on slicing, since the client will deserialize only the first part of this structure
struct ClusterControllerFullInterface {
	ClusterInterface clientInterface;
	RequestStream< struct RecruitFromConfigurationRequest > recruitFromConfiguration;
	RequestStream< struct RecruitStorageRequest > recruitStorage;
	RequestStream< struct RegisterWorkerRequest > registerWorker;
	RequestStream< struct GetWorkersRequest > getWorkers;
	RequestStream< struct RegisterMasterRequest > registerMaster;
	RequestStream< struct GetServerDBInfoRequest > getServerDBInfo;

	UID id() const { return clientInterface.id(); }
	bool operator == (ClusterControllerFullInterface const& r) const { return id() == r.id(); }
	bool operator != (ClusterControllerFullInterface const& r) const { return id() != r.id(); }

	void initEndpoints() {
		clientInterface.initEndpoints();
		recruitFromConfiguration.getEndpoint( TaskClusterController );
		recruitStorage.getEndpoint( TaskClusterController );
		registerWorker.getEndpoint( TaskClusterController );
		getWorkers.getEndpoint( TaskClusterController );
		registerMaster.getEndpoint( TaskClusterController );
		getServerDBInfo.getEndpoint( TaskClusterController );
	}

	template <class Ar>
	void serialize( Ar& ar ) {
		ASSERT( ar.protocolVersion() >= 0x0FDB00A200040001LL );
		ar & clientInterface & recruitFromConfiguration & recruitStorage & registerWorker & getWorkers & registerMaster & getServerDBInfo;
	}
};

struct RecruitFromConfigurationRequest {
	DatabaseConfiguration configuration;
	ReplyPromise< struct RecruitFromConfigurationReply > reply;

	RecruitFromConfigurationRequest() {}
	explicit RecruitFromConfigurationRequest(DatabaseConfiguration const& configuration)
		: configuration(configuration) {}

	template <class Ar>
	void serialize( Ar& ar ) {
		ar & configuration & reply;
	}
};

struct RecruitFromConfigurationReply {
	vector<WorkerInterface> tLogs;
	vector<WorkerInterface> remoteTLogs;
	vector<WorkerInterface> logRouters;
	vector<WorkerInterface> proxies;
	vector<WorkerInterface> resolvers;

	template <class Ar>
	void serialize( Ar& ar ) {
		ar & tLogs & remoteTLogs & logRouters & proxies & resolvers;
	}
};

struct RecruitStorageReply {
	WorkerInterface worker;
	ProcessClass processClass;

	template <class Ar>
	void serialize( Ar& ar ) {
		ar & worker & processClass;
	}
};

struct RecruitStorageRequest {
	std::vector<Optional<Standalone<StringRef>>> excludeMachines;	//< Don't recruit any of these machines
	std::vector<AddressExclusion> excludeAddresses;		//< Don't recruit any of these addresses
	std::vector<Optional<Standalone<StringRef>>> excludeDCs;		//< Don't recruit from any of these data centers
	bool criticalRecruitment;							//< True if machine classes are to be ignored
	ReplyPromise< RecruitStorageReply > reply;

	template <class Ar>
	void serialize( Ar& ar ) {
		ar & excludeMachines & excludeAddresses & excludeDCs & criticalRecruitment & reply;
	}
};

struct RegisterWorkerRequest {
	WorkerInterface wi;
	ProcessClass processClass;
	Generation generation;
	ReplyPromise<Void> reply;

	RegisterWorkerRequest() {}
	RegisterWorkerRequest(WorkerInterface wi, ProcessClass processClass,  Generation generation) : 
	wi(wi), processClass(processClass), generation(generation) {}

	template <class Ar>
	void serialize( Ar& ar ) {
		ar & wi & processClass & generation & reply;
	}
};

struct GetWorkersRequest {
	enum { FLAG_TESTER_CLASS = 1 };

	int flags;
	ReplyPromise<vector<std::pair<WorkerInterface, ProcessClass>>> reply;

	GetWorkersRequest() : flags(0) {}
	explicit GetWorkersRequest(int fl) : flags(fl) {}

	template <class Ar>
	void serialize(Ar& ar) {
		ar & flags & reply;
	}
};

struct RegisterMasterRequest {
	Standalone<StringRef> dbName;
	UID id;
	LocalityData mi;
	LogSystemConfig logSystemConfig;
	vector<MasterProxyInterface> proxies;
	vector<ResolverInterface> resolvers;
	DBRecoveryCount recoveryCount;
	int64_t registrationCount;
	DatabaseConfiguration configuration;
	vector<UID> priorCommittedLogServers;
	int recoveryState;
	
	ReplyPromise<Void> reply;

	RegisterMasterRequest() {}

	template <class Ar>
	void serialize( Ar& ar ) {
		ASSERT( ar.protocolVersion() >= 0x0FDB00A200040001LL );
		ar & dbName & id & mi & logSystemConfig & proxies & resolvers & recoveryCount & registrationCount & configuration & priorCommittedLogServers & recoveryState & reply;
	}
};

struct GetServerDBInfoRequest {
	UID knownServerInfoID;
	Standalone<StringRef> issues;
	std::vector<NetworkAddress> incompatiblePeers;
	ReplyPromise< struct ServerDBInfo > reply;

	template <class Ar>
	void serialize(Ar& ar) {
		ar & knownServerInfoID & issues & incompatiblePeers & reply;
	}
};

#include "ServerDBInfo.h" // include order hack

#endif
