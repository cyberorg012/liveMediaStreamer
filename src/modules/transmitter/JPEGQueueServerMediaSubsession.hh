/*
 *  JPEGQueueServerMediaSubsession.hh - It consumes JPEG frames from the frame queue on demand
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
 *  Authors:  David Cassany <david.cassany@i2cat.net>,
 *            
 */

// #ifndef _JPEG_SERVER_MEDIA_SUBSESSION_HH
// #define _JPEG_SERVER_MEDIA_SUBSESSION_HH
// 
// #include <liveMedia/liveMedia.hh>
// #include "QueueServerMediaSubsession.hh"
// 
// class JPEGQueueServerMediaSubsession: public QueueServerMediaSubsession {
// public:
//   static JPEGQueueServerMediaSubsession*
//   createNew(UsageEnvironment& env, Reader *reader, Boolean reuseFirstSource);
// 
// protected:
//   JPEGQueueServerMediaSubsession(UsageEnvironment& env,
//                                  Reader *reader, Boolean reuseFirstSource);
//       // called only by createNew();
//   virtual ~JPEGQueueServerMediaSubsession(){};
// 
// protected: // redefined virtual functions
//   FramedSource* createNewStreamSource(unsigned clientSessionId,
//                           unsigned& estBitrate);
//   RTPSink* createNewRTPSink(Groupsock* rtpGroupsock,
//                                     unsigned char rtpPayloadTypeIfDynamic,
//                     FramedSource* inputSource);
// };
// 
// #endif

