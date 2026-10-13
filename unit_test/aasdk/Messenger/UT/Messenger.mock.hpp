#pragma once

#include <gmock/gmock.h>
#include <aasdk/Messenger/IMessenger.hpp>


namespace aasdk
{
namespace messenger
{
namespace ut
{

class MessengerMock: public IMessenger
{
public:
    MOCK_METHOD2(enqueueReceive, void(ChannelId channelId, ReceivePromise::Pointer promise));
    MOCK_METHOD2(enqueueSend, void(Message::Pointer message, SendPromise::Pointer promise));
    MOCK_METHOD0(stop, void());
};

}
}
}
