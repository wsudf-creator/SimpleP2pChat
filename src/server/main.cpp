#include "StunServer.h"

void onRequest(StunRequest& req, StunResponse& resp)
{
    if (req.messageType() == STUN_BINDING_REQUEST)
    {   
        resp.setType();
        resp.setResponseHeaders(STUN_BINDING_RESPONSE, 12); //12:Message Length
        resp.setAttrMappedAddress4(0x00);

        // resp.showResponses();
    }
}

int main() 
{
    try 
    {
        boost::asio::io_context io_context;
        StunServer server(io_context, 3478);
        server.setStunCallback(onRequest);
        io_context.run();
    } 
    catch (const std::exception& e) 
    {
        std::cerr << "异常中断: " << e.what() << std::endl;
    }
    return 0;
}