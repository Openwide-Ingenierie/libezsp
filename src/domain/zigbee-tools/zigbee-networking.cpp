/**
 * 
 */

#include <ctime>

#include "zigbee-networking.h"

#include "../ezsp-protocol/get-network-parameters-response.h"
#include "../ezsp-protocol/struct/ember-key-struct.h"
#include "../ezsp-protocol/struct/ember-child-data-struct.h"


CZigbeeNetworking::CZigbeeNetworking( CEzspDongle &i_dongle, CZigbeeMessaging &i_zb_messaging ): dongle(i_dongle), zb_messaging(i_zb_messaging)
{
    dongle.registerObserver(this);
}

void CZigbeeNetworking::handleEzspRxMessage( EEzspCmd i_cmd, std::vector<uint8_t> i_msg_receive )
{
    // std::cout << "CZigbeeNetworking::handleEzspRxMessage : " << CEzspEnum::EEzspCmdToString(i_cmd) << std::endl;

    switch( i_cmd )
    {
        case EZSP_PERMIT_JOINING:
        {
            std::cout << "EZSP_PERMIT_JOINING return status : " << CEzspEnum::EEmberStatusToString(static_cast<EEmberStatus>(i_msg_receive.at(0))) << std::endl;
        }
        break;

        case EZSP_SEND_BROADCAST:
        {
            std::cout << "EZSP_SEND_BROADCAST return status : " << CEzspEnum::EEmberStatusToString(static_cast<EEmberStatus>(i_msg_receive.at(0))) << std::endl;
        }
        break;
        case EZSP_GET_CHILD_DATA:
        {
            std::cout << "EZSP_GET_CHILD_DATA return  at index : " << unsigned(child_idx) << ", status : " << CEzspEnum::EEmberStatusToString(static_cast<EEmberStatus>(i_msg_receive.at(0))) << std::endl;
            if( EMBER_SUCCESS == i_msg_receive.at(0) )
            {
                i_msg_receive.erase(i_msg_receive.begin());
                CEmberChildDataStruct l_rsp(i_msg_receive);
                std::cout << l_rsp.String() << std::endl;

                // appeler la fonction de nouveau produit
                // \todo

                // lire l'entrée suivante
                std::vector<uint8_t> l_param;
                child_idx += 1;
                l_param.push_back(child_idx);
                dongle.sendCommand(EZSP_GET_CHILD_DATA, l_param);
            }

        }
        break;
        case EZSP_SET_INITIAL_SECURITY_STATE:
        {
            std::string status_str = CEzspEnum::EEmberStatusToString(static_cast<EEmberStatus>(i_msg_receive.at(0)));
            std::cout << "EZSP_SET_INITIAL_SECURITY_STATE status : " << status_str << std::endl;
            if( EMBER_SUCCESS == i_msg_receive.at(0) )
            {
                CEmberNetworkParameters payload;

                payload.setPanId(static_cast<uint16_t>(std::rand()&0xFFFF));
                payload.setRadioTxPower(3);
                payload.setRadioChannel(11);
                payload.setJoinMethod(EMBER_USE_MAC_ASSOCIATION);

                dongle.sendCommand(EZSP_FORM_NETWORK, payload.getRaw());
            }
        }
        break;
        case EZSP_FORM_NETWORK:
        {
            std::string status_str = CEzspEnum::EEmberStatusToString(static_cast<EEmberStatus>(i_msg_receive.at(0)));
            std::cout << "EZSP_FORM_NETWORK status : " << status_str << std::endl;
        }
        break;

        default:
        break;
    }

}

void CZigbeeNetworking::formHaNetwork()
{
    // set HA policy
    std::vector<uint8_t> payload;
    uint16_t l_security_bitmak = 0;
    std::srand(std::time(nullptr));

    payload.push_back(EZSP_TRUST_CENTER_POLICY); // EZSP_TRUST_CENTER_POLICY
    payload.push_back(0x01); // EZSP_ALLOW_PRECONFIGURED_KEY_JOINS
    dongle.sendCommand( EZSP_SET_POLICY, payload );

    payload.clear();
    payload.push_back(EZSP_TC_KEY_REQUEST_POLICY); // EZSP_TC_KEY_REQUEST_POLICY
    payload.push_back(0x50); // EZSP_DENY_TC_KEY_REQUESTS
    dongle.sendCommand( EZSP_SET_POLICY, payload );

    // set initial security state
    // EMBER_HAVE_PRECONFIGURED_KEY
    l_security_bitmak |= 0x0100;

    // EMBER_HAVE_NETWORK_KEY
    l_security_bitmak |= 0x0200;

    // EMBER_REQUIRE_ENCRYPTED_KEY
    l_security_bitmak |= 0x0800;

    // EMBER_TRUST_CENTER_GLOBAL_LINK_KEY
    l_security_bitmak |= 0x0004;

    payload.clear();
    // security bitmask
    payload.push_back(static_cast<uint8_t>(l_security_bitmak&0xFF));
    payload.push_back(static_cast<uint8_t>((l_security_bitmak>>8)&0xFF));
    // tc key : HA Key
    payload.push_back(0x5A);
    payload.push_back(0x69);
    payload.push_back(0x67);
    payload.push_back(0x42);
    payload.push_back(0x65);
    payload.push_back(0x65);
    payload.push_back(0x41);
    payload.push_back(0x6C);
    payload.push_back(0x6C);
    payload.push_back(0x69);
    payload.push_back(0x61);
    payload.push_back(0x6E);
    payload.push_back(0x63);
    payload.push_back(0x65);
    payload.push_back(0x30);
    payload.push_back(0x39);
    //  network key : random
    for( uint8_t loop=0; loop<16; loop++ ){ payload.push_back( static_cast<uint8_t>(std::rand()&0xFF )); }
    // key sequence number
    payload.push_back(0U);
    // eui trust center : not used
    for( uint8_t loop=0; loop<8; loop++ ){ payload.push_back( 0U ); }

    // call
    dongle.sendCommand(EZSP_SET_INITIAL_SECURITY_STATE, payload);
}

/**
 * @brief OpenNetwork : open current network
 * @param i_timeout : timeout for open network in second
 * @return true if action is possible
 */
void CZigbeeNetworking::OpenNetwork( uint8_t i_timeout )
{
    std::vector<uint8_t> i_payload;

    i_payload.push_back(i_timeout);
    dongle.sendCommand(EZSP_PERMIT_JOINING, i_payload);

    // use zdp frame
    CZigBeeMsg l_msg;
    std::vector<uint8_t> l_payload;

    l_payload.push_back(i_timeout);
    l_payload.push_back(1);

    l_msg.SetZdo( 0x0036, l_payload );

    zb_messaging.SendBroadcast( E_OUT_MSG_BR_DEST_ALL_DEVICES, 0, l_msg );
}

/**
 * @brief CloseNetwork : open current network
 * @return true if action is possible
 */
void CZigbeeNetworking::CloseNetwork( void )
{
    std::vector<uint8_t> i_payload;

    i_payload.push_back(0);
    dongle.sendCommand(EZSP_PERMIT_JOINING,i_payload);

    // use zdp frame
    CZigBeeMsg l_msg;
    std::vector<uint8_t> l_payload;

    l_payload.push_back(0);
    l_payload.push_back(1);

    l_msg.SetZdo( 0x0036, l_payload );

    zb_messaging.SendBroadcast( E_OUT_MSG_BR_DEST_ALL_DEVICES, 0, l_msg );
}


void CZigbeeNetworking::startDiscoverProduct()
{
    // pour l'exemple on ne lit que la table enfant du dongle, on assume qu'il n'y a pas d'autre routeur dans le réseau
    // lire table enfant du dongle
    std::vector<uint8_t> l_param;
    child_idx = 0;
    l_param.push_back(child_idx);
    dongle.sendCommand(EZSP_GET_CHILD_DATA, l_param);
    
}