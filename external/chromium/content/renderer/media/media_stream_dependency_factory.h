// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_RENDERER_MEDIA_MEDIA_STREAM_DEPENDENCY_FACTORY_H_
#define CONTENT_RENDERER_MEDIA_MEDIA_STREAM_DEPENDENCY_FACTORY_H_

#include <string>

#include "base/basictypes.h"
#include "base/memory/ref_counted.h"
#include "base/threading/thread.h"
#include "content/common/content_export.h"
#include "content/renderer/media/media_stream_extra_data.h"
#include "content/renderer/p2p/socket_dispatcher.h"
#include "third_party/libjingle/source/talk/app/webrtc/peerconnectioninterface.h"
#include "third_party/libjingle/source/talk/app/webrtc/videosourceinterface.h"

namespace base {
class WaitableEvent;
}

namespace talk_base {
class NetworkManager;
class PacketSocketFactory;
class Thread;
}

namespace webrtc {
class PeerConnection;
}

namespace WebKit {
class WebFrame;
class WebMediaConstraints;
class WebMediaStreamDescriptor;
class WebPeerConnection00Handler;
class WebPeerConnection00HandlerClient;
class WebRTCPeerConnectionHandler;
class WebRTCPeerConnectionHandlerClient;
}

namespace content {

class IpcNetworkManager;
class IpcPacketSocketFactory;
class VideoCaptureImplManager;
class WebRtcAudioDeviceImpl;

// Object factory for RTC MediaStreams and RTC PeerConnections.
class CONTENT_EXPORT MediaStreamDependencyFactory
    : NON_EXPORTED_BASE(public base::NonThreadSafe) {
 public:
  // MediaSourcesCreatedCallback is used in CreateNativeMediaSources.
  typedef base::Callback<void(WebKit::WebMediaStreamDescriptor* description,
                              bool live)> MediaSourcesCreatedCallback;
  MediaStreamDependencyFactory(
      VideoCaptureImplManager* vc_manager,
      P2PSocketDispatcher* p2p_socket_dispatcher);
  virtual ~MediaStreamDependencyFactory();

  // Create a PeerConnectionHandlerJsep object that implements the
  // WebKit WebPeerConnection00Handler interface.
  WebKit::WebPeerConnection00Handler* CreatePeerConnectionHandlerJsep(
      WebKit::WebPeerConnection00HandlerClient* client);

  // Create a RTCPeerConnectionHandler object that implements the
  // WebKit WebRTCPeerConnectionHandler interface.
  WebKit::WebRTCPeerConnectionHandler* CreateRTCPeerConnectionHandler(
      WebKit::WebRTCPeerConnectionHandlerClient* client);

  // CreateNativeMediaSources creates libjingle representations of
  // the underlying sources to the tracks in |description|.
  // |sources_created| is invoked when the sources have either been created and
  // transitioned to a live state or failed.
  // The libjingle sources is stored in the extra data field of
  // WebMediaStreamSource.
  // |audio_constraints| and |video_constraints| set parameters for the sources.
  void CreateNativeMediaSources(
      const WebKit::WebMediaConstraints& audio_constraints,
      const WebKit::WebMediaConstraints& video_constraints,
      WebKit::WebMediaStreamDescriptor* description,
      const MediaSourcesCreatedCallback& sources_created);

  // Creates a libjingle representation of a MediaStream and stores
  // it in the extra data field of |description|.
  void CreateNativeLocalMediaStream(
      WebKit::WebMediaStreamDescriptor* description);

  // Creates a libjingle representation of a MediaStream and stores
  // it in the extra data field of |description|.
  // |stream_stopped| is a callback that is run when a MediaStream have been
  // stopped.
  void CreateNativeLocalMediaStream(
      WebKit::WebMediaStreamDescriptor* description,
      const MediaStreamExtraData::StreamStopCallback& stream_stop);

  // Asks the libjingle PeerConnection factory to create a libjingle
  // PeerConnection object.
  // The PeerConnection object is owned by PeerConnectionHandler.
  virtual scoped_refptr<webrtc::PeerConnectionInterface>
      CreatePeerConnection(const webrtc::JsepInterface::IceServers& ice_servers,
                           const webrtc::MediaConstraintsInterface* constraints,
                           WebKit::WebFrame* web_frame,
                           webrtc::PeerConnectionObserver* observer);

  // Creates a libjingle representation of a Session description. Used by a
  // PeerConnectionHandlerJsep instance.
  virtual webrtc::SessionDescriptionInterface* CreateSessionDescription(
      const std::string& sdp);

  // Creates a libjingle representation of a Session description. Used by a
  // RTCPeerConnectionHandler instance.
  virtual webrtc::SessionDescriptionInterface* CreateSessionDescription(
      const std::string& type,
      const std::string& sdp);

  // Creates a libjingle representation of an ice candidate.
  virtual webrtc::IceCandidateInterface* CreateIceCandidate(
      const std::string& sdp_mid,
      int sdp_mline_index,
      const std::string& sdp);

  WebRtcAudioDeviceImpl* GetWebRtcAudioDevice();

 protected:
  // Asks the PeerConnection factory to create a Local MediaStream object.
  virtual scoped_refptr<webrtc::LocalMediaStreamInterface>
      CreateLocalMediaStream(const std::string& label);

  // Asks the PeerConnection factory to create a Local Video Source.
  virtual scoped_refptr<webrtc::VideoSourceInterface>
      CreateVideoSource(int video_session_id,
                        bool is_screen_cast,
                        const webrtc::MediaConstraintsInterface* constraints);

  // Asks the PeerConnection factory to create a Local VideoTrack object.
  virtual scoped_refptr<webrtc::VideoTrackInterface>
      CreateLocalVideoTrack(const std::string& label,
                            webrtc::VideoSourceInterface* source);

  // Asks the PeerConnection factory to create a Local AudioTrack object.
  virtual scoped_refptr<webrtc::LocalAudioTrackInterface>
      CreateLocalAudioTrack(const std::string& label,
                            webrtc::AudioDeviceModule* audio_device);

  virtual bool EnsurePeerConnectionFactory();
  virtual bool PeerConnectionFactoryCreated();
  virtual void SetAudioDeviceSessionId(int session_id);

 private:
  // Creates and deletes |pc_factory_|, which in turn is used for
  // creating PeerConnection objects.
  bool CreatePeerConnectionFactory();

  void InitializeWorkerThread(talk_base::Thread** thread,
                              base::WaitableEvent* event);

  void CreateIpcNetworkManagerOnWorkerThread(base::WaitableEvent* event);
  void DeleteIpcNetworkManager();
  void CleanupPeerConnectionFactory();

  // We own network_manager_, must be deleted on the worker thread.
  // The network manager uses |p2p_socket_dispatcher_|.
  IpcNetworkManager* network_manager_;
  scoped_ptr<IpcPacketSocketFactory> socket_factory_;

  scoped_refptr<webrtc::PeerConnectionFactoryInterface> pc_factory_;

  scoped_refptr<VideoCaptureImplManager> vc_manager_;
  scoped_refptr<P2PSocketDispatcher> p2p_socket_dispatcher_;
  scoped_refptr<WebRtcAudioDeviceImpl> audio_device_;

  // PeerConnection threads. signaling_thread_ is created from the
  // "current" chrome thread.
  talk_base::Thread* signaling_thread_;
  talk_base::Thread* worker_thread_;
  base::Thread chrome_worker_thread_;

  DISALLOW_COPY_AND_ASSIGN(MediaStreamDependencyFactory);
};

}  // namespace content

#endif  // CONTENT_RENDERER_MEDIA_MEDIA_STREAM_DEPENDENCY_FACTORY_H_
