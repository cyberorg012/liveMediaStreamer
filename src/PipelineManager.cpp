/*
 *  PipelineManager.cpp - Pipeline manager class for livemediastreamer framework
 *  Copyright (C) 2014  Fundació i2CAT, Internet i Innovació digital a Catalunya
 *
 *  This file is part of liveMediaStreamer.
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *  Authors:  Marc Palau <marc.palau@i2cat.net>,
 *            David Cassany <david.cassany@i2cat.net>
 */

#include "PipelineManager.hh"

PipelineManager::PipelineManager()
{
    pipeMngrInstance = this;
    receiverID = rand();
    transmitterID = rand();
    addFilter(receiverID, SourceManager::getInstance());
    addFilter(transmitterID, SinkManager::getInstance());

    LiveMediaWorker *receiverWorker = new LiveMediaWorker();
    receiverWorker->addProcessor(receiverID, SourceManager::getInstance());

    LiveMediaWorker *transmitterWorker = new LiveMediaWorker();
    transmitterWorker->addProcessor(transmitterID, SinkManager::getInstance());
}

PipelineManager* PipelineManager::getInstance()
{
    if (pipeMngrInstance != NULL) {
        return pipeMngrInstance;
    }

    return new PipelineManager();
}

void PipelineManager::destroyInstance()
{
    PipelineManager* pipeMngrInstance = PipelineManager::getInstance();
    if (pipeMngrInstance != NULL) {
        delete pipeMngrInstance;
        pipeMngrInstance = NULL;
    }
}

int PipelineManager::searchFilterIDByType(FilterType type)
{
    for (auto it : filters) {
        if (it.second->getType() == type) {
            return it.first;
        }
    }

    return -1;
}

bool PipelineManager::addPath(int id, Path* path)
{
    if (paths.count(id) > 0) {
        return false;
    }

    paths[id] = path;

    return true;
}

BaseFilter* PipelineManager::createFilter(FilterType type)
{
    BaseFilter* filter = NULL;

    switch (type) {
        case VIDEO_DECODER:
            filter = new VideoDecoderLibav();
            break;
        case VIDEO_ENCODER:
            filter = new VideoEncoderX264();
            break;
        case VIDEO_MIXER:
            filter = new VideoMixer();
            break;
        case VIDEO_RESAMPLER:
            filter = new VideoResampler();
            break;
        case AUDIO_DECODER:
            filter = new AudioDecoderLibav();
            break;
        case AUDIO_ENCODER:
            filter = new AudioEncoderLibav();
            break;
        case AUDIO_MIXER:
            filter = new AudioMixer();
            break;
    }

    return filter;
}


bool PipelineManager::addFilter(int id, BaseFilter* filter)
{
    if (filters.count(id) > 0) {
        return false;
    }

    filters[id] = filter;

    return true;
}

BaseFilter* PipelineManager::getFilter(int id)
{
    if (filters.count(id) <= 0) {
        return NULL;
    }

    return filters[id];
}

bool PipelineManager::addWorker(int id, Worker* worker)
{
    if (workers.count(id) > 0) {
        return false;
    }

    workers[id] = worker;

    return true;
}

Worker* PipelineManager::getWorker(int id)
{
    if (workers.count(id) <= 0) {
        return NULL;
    }
    
    return workers[id];
}

bool PipelineManager::addFilterToWorker(int workerId, int filterId)
{
    if (filters.count(filterId) <= 0 || workers.count(workerId) <= 0) {
        return false;
    }

    filters[filterId]->setWorkerId(workerId);

    if (!workers[workerId]->addProcessor(filterId, filters[filterId])) {
        return false;
    }

    return true;
}

Path* PipelineManager::getPath(int id)
{
    if (paths.count(id) <= 0) {
        return NULL;
    }

    return paths[id];
}

Path* PipelineManager::createPath(int orgFilter, int dstFilter, int orgWriter, int dstReader, std::vector<int> midFilters, bool sharedQueue)
{
    Path* path;
    BaseFilter* originFilter;
    BaseFilter* destinationFilter;
    int realOrgWriter = orgWriter;
    int realDstReader = dstReader;

    if (filters.count(orgFilter) <= 0 || filters.count(dstFilter) <= 0) {
        return NULL;
    }

    for (auto it : midFilters) {
        if (filters.count(it) <= 0) {
            return NULL;
        }
    }

    originFilter = filters[orgFilter];
    destinationFilter = filters[dstFilter];

    if (realOrgWriter < 0) {
        realOrgWriter = originFilter->generateWriterID();
    }

    if (realDstReader < 0) {
        realDstReader = destinationFilter->generateReaderID();
    }

    path = new Path(orgFilter, dstFilter, realOrgWriter, realDstReader, midFilters, sharedQueue); 

    return path;
}


bool PipelineManager::connectPath(Path* path)
{
    int orgFilterId = path->getOriginFilterID();
    int dstFilterId = path->getDestinationFilterID();
    
    std::vector<int> pathFilters = path->getFilters();

    if (pathFilters.empty()) {
        if (filters[orgFilterId]->connectManyToMany(filters[dstFilterId], 
                                                    path->getDstReaderID(), 
                                                    path->getOrgWriterID(),
                                                    path->getShared()
                                                    )) 
        {
            return true;
        } else {
            utils::errorMsg("Connecting head to tail!");
            return false;
        }
    }

    if (!filters[orgFilterId]->connectManyToOne(filters[pathFilters.front()], path->getOrgWriterID(), path->getShared())) {
        utils::errorMsg("Connecting path head to first filter!");
        return false;
    }

    for (unsigned i = 0; i < pathFilters.size() - 1; i++) {
        if (!filters[pathFilters[i]]->connectOneToOne(filters[pathFilters[i+1]])) {
            utils::errorMsg("Connecting path filters!");
            return false;
        }
    }

    if (!filters[pathFilters.back()]->connectOneToMany(filters[dstFilterId], path->getDstReaderID())) {
        utils::errorMsg("Connecting path last filter to path tail!");
        return false;
    }

    return true;
}

bool PipelineManager::removePath(int id)
{
    Path* path = NULL;

    path = getPath(id);

    if (!path) {
        return false;
    }

    if (!deletePath(path)) {
        return false;
    }

    paths.erase(id);

    return true;
}


bool PipelineManager::deletePath(Path* path) 
{
    std::vector<int> pathFilters = path->getFilters();
    int orgFilterID = path->getOriginFilterID();
    int dstFilterID = path->getDestinationFilterID();

    if (filters.count(orgFilterID) <= 0 || filters.count(dstFilterID) <= 0) {
        return false;
    }

    for (auto it : pathFilters) {
        if (filters.count(it) <= 0) {
            return false;
        }
    }

    if (!filters[orgFilterID]->disconnect(filters[pathFilters.front()], path->getOrgWriterID(), DEFAULT_ID)) {
        utils::errorMsg("Error disconnecting path head from first filter!");
        return false;
    }


    for (unsigned i = 0; i < pathFilters.size() - 1; i++) {
        if (!filters[pathFilters[i]]->disconnect(filters[pathFilters[i+1]], DEFAULT_ID, DEFAULT_ID)) {
            utils::errorMsg("Error disconnecting path filters!");
            return false;
        }
    }

    if (!filters[pathFilters.back()]->disconnect(filters[dstFilterID], DEFAULT_ID, path->getDstReaderID())) {
        utils::errorMsg("Error disconnecting path last filter to path tail!");
        return false;
    }

    for (auto it : pathFilters) {
        workers[filters[it]->getWorkerId()]->removeProcessor(it);
        delete filters[it];
        filters.erase(it);
    }

    delete path;

    return true;
}

void PipelineManager::startWorkers()
{   
    for (auto it : workers) {
        if (!it.second->isRunning()) {
            it.second->start();
            utils::debugMsg("Worker " + std::to_string(it.first) + " started");
        }
    }
}

void PipelineManager::stopWorkers()
{
    for (auto it : workers) {
        if (it.second->isRunning()) {
            it.second->stop();
            utils::debugMsg("Worker " + std::to_string(it.first) + " stoped");
        }
    }
}

SourceManager* PipelineManager::getReceiver()
{
    return dynamic_cast<SourceManager*>(filters[receiverID]);
}


SinkManager* PipelineManager::getTransmitter() 
{
    return dynamic_cast<SinkManager*>(filters[transmitterID]);
}

void PipelineManager::getStateEvent(Jzon::Node* params, Jzon::Object &outputNode)
{
    Jzon::Array filterList;
    Jzon::Array pathList;
    Jzon::Array workersList;

    for (auto it : filters) {
        Jzon::Object filter;
        filter.Add("id", it.first);
        it.second->getState(filter);
        filterList.Add(filter);
    }

    outputNode.Add("filters", filterList);

    for (auto it : paths) {
        Jzon::Object path;
        Jzon::Array pathFilters;
        std::vector<int> pFilters = it.second->getFilters();

        path.Add("id", it.first);
        path.Add("originFilter", it.second->getOriginFilterID());
        path.Add("destinationFilter", it.second->getDestinationFilterID());
        path.Add("originWriter", it.second->getOrgWriterID());
        path.Add("destinationReader", it.second->getDstReaderID());

        for (auto it : pFilters) {
            pathFilters.Add(it);
        }

        path.Add("filters", pathFilters);
        pathList.Add(path);
    }

    outputNode.Add("paths", pathList);
    
    for (auto it : workers) {
        Jzon::Object worker;
        worker.Add("id", it.first);
        workersList.Add(worker);
    }

    outputNode.Add("workers", workersList);

}

void PipelineManager::reconfigAudioEncoderEvent(Jzon::Node* params, Jzon::Object &outputNode)
{
    int encoderID, mixerID, pathID;
    int sampleRate, channels;
    Path* path;
    ACodecType codec;
    std::string sCodec;
    SinkManager* transmitter = getTransmitter();

    if (!params->Has("encoderID") || !params->Has("codec") || !params->Has("sampleRate") || !params->Has("channels")) {
        outputNode.Add("error", "Error configure audio encoder. Encoder ID is not valid");
        return;
    }

    encoderID = params->Get("encoderID").ToInt();
    sampleRate = params->Get("sampleRate").ToInt();
    channels = params->Get("channels").ToInt();
    sCodec = params->Get("codec").ToString();
    codec = utils::getCodecFromString(sCodec);

    for (auto it : paths) {
        if (it.second->getFilters().front() == encoderID) {
            pathID = it.first;
            path = it.second;
        }
    }

    if (!path) {
        outputNode.Add("error", "Error reconfiguring audio encoder");
        return;
    }

    mixerID = path->getOriginFilterID();

    if (!removePath(pathID)) {
        outputNode.Add("error", "Error reconfiguring audio encoder");
        return;
    }

    path = new AudioEncoderPath(mixerID, getFilter(mixerID)->generateWriterID());
    dynamic_cast<AudioEncoderLibav*>(getFilter(path->getFilters().front()))->configure(codec, channels, sampleRate);

    path->setDestinationFilter(transmitterID, transmitter->generateReaderID());

    if (!connectPath(path)) {
        outputNode.Add("error", "Error configure audio encoder. Encoder ID is not valid");
        return;
    }

    int encoderPathID = rand();

    if (!addPath(encoderPathID, path)) {
        outputNode.Add("error", "Error configure audio encoder. Encoder ID is not valid");
        return;
    }

    outputNode.Add("error", Jzon::null);

}

void PipelineManager::createFilterEvent(Jzon::Node* params, Jzon::Object &outputNode)
{
    int id;
    FilterType fType;
    BaseFilter* filter;

    if(!params) {
        outputNode.Add("error", "Error creating filter. Invalid JSON format...");
        return;
    }

    if (!params->Has("id") || !params->Has("type")) {
        outputNode.Add("error", "Error creating filter. Invalid JSON format...");
        return;
    }

    id = params->Get("id").ToInt();
    fType = utils::getFilterTypeFromString(params->Get("type").ToString());

    filter = createFilter(fType);

    if (!filter) {
        outputNode.Add("error", "Error creating filter. Specified type is not correct..");
        return;
    }

    if (!addFilter(id, filter)) {
        outputNode.Add("error", "Error registering filter. Specified ID already exists..");
        return;
    }

    outputNode.Add("error", Jzon::null);
}

void PipelineManager::createPathEvent(Jzon::Node* params, Jzon::Object &outputNode) 
{
    std::vector<int> filtersIds;
    int id, orgFilterId, dstFilterId;
    int orgWriterId = -1;
    int dstReaderId = -1;
    bool sharedQueue = false;
    Path* path;

    if(!params) {
        outputNode.Add("error", "Error creating path. Invalid JSON format...");
        return;
    }

    if (!params->Has("id") || !params->Has("orgFilterId") || 
          !params->Has("dstFilterId") || !params->Has("orgWriterId") || 
            !params->Has("dstReaderId") || !params->Has("sharedQueue")) {
        outputNode.Add("error", "Error creating path. Invalid JSON format...");
        return;
    }

   if (!params->Has("midFiltersIds") || !params->Get("midFiltersIds").IsArray()) {
      outputNode.Add("error", "Error creating path. Invalid JSON format...");
      return;
   }
        
    Jzon::Array& jsonFiltersIds = params->Get("midFiltersIds").AsArray();
    id = params->Get("id").ToInt();
    orgFilterId = params->Get("orgFilterId").ToInt();
    dstFilterId = params->Get("dstFilterId").ToInt();
    orgWriterId = params->Get("orgWriterId").ToInt();
    dstReaderId = params->Get("dstReaderId").ToInt();
    sharedQueue = params->Get("sharedQueue").ToBool();

    for (Jzon::Array::iterator it = jsonFiltersIds.begin(); it != jsonFiltersIds.end(); ++it) {
        filtersIds.push_back((*it).ToInt());
    }

    path = createPath(orgFilterId, dstFilterId, orgWriterId, dstReaderId, filtersIds, sharedQueue);

    if (!path) {
        outputNode.Add("error", "Error creating path. Check introduced filter IDs...");
        return;
    }

    if (!connectPath(path)) {
        outputNode.Add("error", "Error connecting path. Better pray Jesus...");
        return;
    }

    if (!addPath(id, path)) {
        outputNode.Add("error", "Error registering path. Path ID already exists...");
        return;
    }

    outputNode.Add("error", Jzon::null);
}

void PipelineManager::addWorkerEvent(Jzon::Node* params, Jzon::Object &outputNode) 
{
    int id, fps;
    std::string type;
    Worker* worker = NULL;

    if(!params) {
        outputNode.Add("error", "Error creating worker. Invalid JSON format...");
        return;
    }

    if (!params->Has("id") || !params->Has("type") || !params->Has("fps")) {
        outputNode.Add("error", "Error creating path. Invalid JSON format...");
        return;
    }

    id = params->Get("id").ToInt();
    type = params->Get("type").ToString();
    fps = params->Get("fps").ToInt();

    if (type.compare("bestEffortMaster") == 0) {
        worker = new BestEffortMaster();
    } else if (type.compare("bestEffortSlave") == 0) {
        worker = new BestEffortSlave();
    } else if (type.compare("constantFramerateMaster") == 0) {
        worker = new ConstantFramerateMaster();
    } else if (type.compare("constantFramerateMaster") == 0) {
        worker = new ConstantFramerateSlave();
    }

    if (!worker) {
        outputNode.Add("error", "Error creating worker. Check type...");
        return;
    }

    if (!addWorker(id, worker)) {
        outputNode.Add("error", "Error adding worker to filter. Check filter ID...");
        return;
    }

    startWorkers();

    outputNode.Add("error", Jzon::null);
}

void PipelineManager::addSlavesToWorkerEvent(Jzon::Node* params, Jzon::Object &outputNode) 
{
    Master* master = NULL;
    std::vector<Worker*> slaves;
    int masterId;

    if(!params) {
        outputNode.Add("error", "Error adding slaves to worker. Invalid JSON format...");
        return;
    }

    if (!params->Has("master")) {
        outputNode.Add("error", "Error adding slaves to worker. Invalid JSON format...");
        return;
    }

    if (!params->Has("slaves") || !params->Get("slaves").IsArray()) {
        outputNode.Add("error", "Error adding slaves to worker. Invalid JSON format...");
        return;
    }

    masterId = params->Get("master").ToInt();
    Jzon::Array& jsonSlavesIds = params->Get("slaves").AsArray();

    master = dynamic_cast<Master*>(getWorker(masterId));

    if (!master) {
        outputNode.Add("error", "Error adding slaves to worker. Invalid Master ID...");
        return;
    }

    for (Jzon::Array::iterator it = jsonSlavesIds.begin(); it != jsonSlavesIds.end(); ++it) {
        master->addSlave((*it).ToInt(), dynamic_cast<Slave*>(workers[(*it).ToInt()]));
    }

    startWorkers();

    outputNode.Add("error", Jzon::null);
}

void PipelineManager::addFiltersToWorkerEvent(Jzon::Node* params, Jzon::Object &outputNode)
{
    int workerId;
    Worker *worker = NULL;

    if(!params) {
        outputNode.Add("error", "Error adding slaves to worker. Invalid JSON format...");
        return;
    }

    if (!params->Has("worker")) {
        outputNode.Add("error", "Error adding filters to worker. Invalid JSON format...");
        return;
    }

    if (!params->Has("filters") || !params->Get("filters").IsArray()) {
        outputNode.Add("error", "Error adding filters to worker. Invalid JSON format...");
        return;
    }

    workerId = params->Get("worker").ToInt();
    Jzon::Array& jsonFiltersIds = params->Get("filters").AsArray();

    for (Jzon::Array::iterator it = jsonFiltersIds.begin(); it != jsonFiltersIds.end(); ++it) {

        if (!addFilterToWorker(workerId, (*it).ToInt())) {
            outputNode.Add("error", "Error adding filters to worker. Invalid internal error...");
            return;
        }
    }

    startWorkers();

    outputNode.Add("error", Jzon::null);
}