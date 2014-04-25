#include "QueueSource.hh"
#include <iostream>
#include <stdio.h>

QueueSource* QueueSource::createNew(UsageEnvironment& env, FrameQueue *q) {
  
  return new QueueSource(env, q);
}


QueueSource::QueueSource(UsageEnvironment& env, FrameQueue *q)
  : FramedSource(env), queue(q) {
}

void QueueSource::doGetNextFrame() {
    if ((frame = queue->getFront()) == NULL) {
        nextTask() = envir().taskScheduler().scheduleDelayedTask(0,
            (TaskFunc*)QueueSource::staticDoGetNextFrame, this);
        return;
    }

    memcpy(fTo, frame->getBuffer(), frame->getBufferLen());
    fFrameSize = frame->getBufferLen(); // out
    gettimeofday(&fPresentationTime, NULL); // out
    fNumTruncatedBytes = 0; // out
    queue->removeFrame();

    afterGetting(this);
}

void QueueSource::doStopGettingFrames() {
  return;
}

void QueueSource::staticDoGetNextFrame(FramedSource* source) {
    source->doGetNextFrame();
}
