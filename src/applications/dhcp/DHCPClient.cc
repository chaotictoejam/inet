//
// Copyright (C) 2008 Juan-Carlos Maureira
// Copyright (C) INRIA
// Copyright (C) 2013 OpenSim Ltd.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this program; if not, see <http://www.gnu.org/licenses/>.
//

#include "DHCPClient.h"

#include "InterfaceTableAccess.h"
#include "RoutingTableAccess.h"
#include "IPv4InterfaceData.h"
#include "NodeStatus.h"
#include "NotifierConsts.h"
#include "NodeOperations.h"

Define_Module(DHCPClient);

DHCPClient::DHCPClient()
{
    timerT1 = NULL;
    timerT2 = NULL;
    timerTo = NULL;
    leaseTimer = NULL;
    startTimer = NULL;
    nb = NULL;
    ie = NULL;
    irt = NULL;
    lease = NULL;
}

DHCPClient::~DHCPClient()
{
    cancelAndDelete(timerT1);
    cancelAndDelete(timerTo);
    cancelAndDelete(timerT2);
    cancelAndDelete(leaseTimer);
    cancelAndDelete(startTimer);
}

void DHCPClient::initialize(int stage)
{
    if (stage == 1)
    {
        timerT1 = new cMessage("T1 Timer",T1);
        timerT2 = new cMessage("T2 Timer",T2);
        timerTo = new cMessage("DHCP Timeout");
        leaseTimer = new cMessage("Lease Timeout",LEASE_TIMEOUT);
        startTimer = new cMessage("Starting DHCP",START_DHCP);

        NodeStatus *nodeStatus = dynamic_cast<NodeStatus *>(findContainingNode(this)->getSubmodule("status"));
        isOperational = (!nodeStatus) || nodeStatus->getState() == NodeStatus::UP;
    }
    else if (stage == 3)
    {
        numSent = 0;
        numReceived = 0;
        xid = 0;
        responseTimeout = 60; // response timeout in seconds RFC 2131, 4.4.3

        WATCH(numSent);
        WATCH(numReceived);
        WATCH(clientState);
        WATCH(xid);

        // DHCP UDP ports
        clientPort = 68; // client
        serverPort = 67; // server

        // get the hostname
        cModule *host = findContainingNode(this);
        hostName = host->getFullName();
        nb = check_and_cast<NotificationBoard *>(getModuleByPath(par("notificationBoardPath")));

        // for a wireless interface subscribe the association event to start the DHCP protocol
        nb->subscribe(this, NF_L2_ASSOCIATED);
        nb->subscribe(this, NF_INTERFACE_DELETED);

        // get the interface to configure
        if (isOperational)
            ie = chooseInterface();

        // get the routing table to update and subscribe it to the blackboard
        irt = check_and_cast<IRoutingTable*>(getModuleByPath(par("routingTablePath")));

        // grab the interface MAC address
        macAddress = ie->getMacAddress();

        // set client to idle state
        clientState = IDLE;

        // FIXME following line is a HACK. It allows to work with all type of interfaces (not just wireless)
        // a correct fix would need some kind of notification when the wireless interface is associated
        // or when the eth interface gets connected and would set the INIT state only then. At the moment
        // there is no such notification in INET.
        // initClient();
        scheduleAt(simTime() + par("startTime").doubleValue(), startTimer);
    }
}

InterfaceEntry *DHCPClient::chooseInterface()
{
    IInterfaceTable* ift = check_and_cast<IInterfaceTable*>(getModuleByPath(par("interfaceTablePath")));
    const char *interfaceName = par("interface");
    InterfaceEntry *ie = NULL;

    if (strlen(interfaceName) > 0)
    {
        ie = ift->getInterfaceByName(interfaceName);
        if (ie == NULL)
            throw new cRuntimeError("Interface \"%s\" does not exist", interfaceName);
    }
    else
    {
        // there should be exactly one non-loopback interface that we want to configure
        for (int i = 0; i < ift->getNumInterfaces(); i++) {
            InterfaceEntry *current = ift->getInterface(i);
            if (!current->isLoopback()) {
                if (ie)
                    throw new cRuntimeError("Multiple non-loopback interfaces found, please select explicitly which one you want to configure via DHCP");
                ie = current;
            }
        }
        if (!ie)
            throw new cRuntimeError("No non-loopback interface found to be configured via DHCP");
    }

    if (!ie->ipv4Data()->getIPAddress().isUnspecified())
        throw new cRuntimeError("Refusing to start DHCP on interface \"%s\" that already has an IP address", ie->getName());
    return ie;
}

void DHCPClient::finish()
{
    cancelEvent(timerT1);
    cancelEvent(timerTo);
    cancelEvent(timerT2);
    cancelEvent(leaseTimer);
    cancelEvent(startTimer);
}

static bool routeMatches(const IPv4Route *entry, const IPv4Address& target, const IPv4Address& nmask,
        const IPv4Address& gw, int metric, const char *dev)
{
    if (!target.isUnspecified() && !target.equals(entry->getDestination()))
        return false;
    if (!nmask.isUnspecified() && !nmask.equals(entry->getNetmask()))
        return false;
    if (!gw.isUnspecified() && !gw.equals(entry->getGateway()))
        return false;
    if (metric && metric != entry->getMetric())
        return false;
    if (dev && strcmp(dev, entry->getInterfaceName()))
        return false;

    return true;
}

const char *DHCPClient::getStateName(ClientState state)
{
    switch (state)
    {
#define CASE(X)  case X: return #X;
    CASE(INIT);
    CASE(INIT_REBOOT);
    CASE(REBOOTING);
    CASE(SELECTING);
    CASE(REQUESTING);
    CASE(BOUND);
    CASE(RENEWING);
    CASE(REBINDING);
    default: return "???";
#undef CASE
    }
}

void DHCPClient::updateDisplayString()
{
    getDisplayString().setTagArg("t", 0, getStateName(clientState));
}

void DHCPClient::handleMessage(cMessage *msg)
{
    if (!isOperational)
    {
        EV_ERROR << "Message '" << msg << "' arrived when module status is down, dropping." << endl;
        delete msg;
        return;
    }
    if (msg->isSelfMessage())
    {
        if (msg->getKind() == START_DHCP)
        {
            openSocket();
            initClient();
        }
        else
            handleTimer(msg);
    }
    else if (msg->arrivedOn("udpIn"))
    {
        DHCPMessage *dhcpPacket = dynamic_cast<DHCPMessage*>(msg);
        if (!dhcpPacket)
            throw cRuntimeError(dhcpPacket, "Unexpected packet received (not a DHCPMessage)");

        handleDHCPMessage(dhcpPacket);
        delete msg;
    }

    if (ev.isGUI())
        updateDisplayString();
}

void DHCPClient::handleTimer(cMessage * msg)
{
    int category = msg->getKind();

    if (category == WAIT_OFFER)
    {
        EV_DETAIL << "No DHCP offer received within timeout. Restarting. " << endl;
        initClient();
    }
    else if (category == WAIT_ACK)
    {
        EV_DETAIL << "No DHCP ACK received within timeout. Restarting." << endl;
        initClient();
    }
    else if (category == T1)
    {
        EV_DETAIL << "T1 expired. Starting RENEWING state." << endl;
        clientState = RENEWING;
        scheduleTimerTO(WAIT_ACK);
        sendRequest();
    }
    else if (category == T2 && clientState == RENEWING)
    {
        EV_DETAIL << "T2 expired. Starting REBINDING state." << endl;
        clientState = REBINDING;
        cancelEvent(timerT1);
        cancelEvent(timerT2);
        cancelEvent(timerTo);
        cancelEvent(leaseTimer);

        sendRequest();
        scheduleTimerTO(WAIT_ACK);
    }
    else if (category == LEASE_TIMEOUT)
    {
        EV_INFO << "Lease has expired. Starting DHCP process in INIT state." << endl;
        unboundLease();
        clientState = INIT;
        initClient();
    }

}

void DHCPClient::recordOffer(DHCPMessage * dhcpOffer)
{
    if (!dhcpOffer->getYiaddr().isUnspecified())
     {
         IPv4Address ip = dhcpOffer->getYiaddr();

         //Byte serverIdB = dhcpOffer->getOptions().get(SERVER_ID);
         IPv4Address serverId = dhcpOffer->getOptions().getServerIdentifier();

         // minimal information to configure the interface
         if (!ip.isUnspecified())
         {
             // create the lease to request
             lease = new DHCPLease();
             lease->ip = ip;
             lease->mac = macAddress;
             lease->serverId = serverId;
         }
     }
     else
         EV_WARN << "DHCPOFFER arrived, but no IP address has been offered. Discarding it and remaining in SELECTING." << endl;
}

void DHCPClient::recordLease(DHCPMessage * dhcpACK)
{
    if (!dhcpACK->getYiaddr().isUnspecified())
    {
        IPv4Address ip = dhcpACK->getYiaddr();
        EV_DETAIL << "DHCPACK arrived with " << "IP: " << ip << endl;

        lease->subnetMask = dhcpACK->getOptions().getSubnetMask();

        if (dhcpACK->getOptions().getRouterArraySize() > 0)
            lease->gateway = dhcpACK->getOptions().getRouter(0);
        if (dhcpACK->getOptions().getDnsArraySize() > 0)
            lease->dns = dhcpACK->getOptions().getDns(0);
        if (dhcpACK->getOptions().getNtpArraySize() > 0)
            lease->ntp = dhcpACK->getOptions().getNtp(0);

        lease->leaseTime = dhcpACK->getOptions().getLeaseTime();
        lease->renewalTime = dhcpACK->getOptions().getRenewalTime();
        lease->rebindTime = dhcpACK->getOptions().getRebindingTime();

        // std::cout << lease->leaseTime << " " << lease->renewalTime << " " << lease->rebindTime << endl;

    }
    else
        EV_ERROR << "DHCPACK arrived, but no IP address confirmed." << endl;
}

void DHCPClient::boundLease()
{
    ie->ipv4Data()->setIPAddress(lease->ip);
    ie->ipv4Data()->setNetmask(lease->ip.getNetworkMask());

    std::string banner = "Got IP " + lease->ip.str();
    this->getParentModule()->bubble(banner.c_str());

    EV_INFO << "Configuring interface : " << ie->getName() << " IP:" << lease->ip << "/"
            << lease->subnetMask << " leased time: " << lease->leaseTime << " (secs)." << endl;

    std::cout << "Host " << hostName << " got ip: " << lease->ip << "/" << lease->subnetMask << endl;

    IPv4Route * iroute = NULL;
    for (int i = 0; i < irt->getNumRoutes(); i++)
    {
        IPv4Route * e = irt->getRoute(i);
        if (routeMatches(e, IPv4Address(), IPv4Address(), lease->gateway, 0, ie->getName()))
        {
            iroute = e;
            break;
        }
    }
    if (iroute == NULL)
    {
        // create gateway route
        IPv4Route *e = new IPv4Route();
        e->setDestination(IPv4Address());
        e->setNetmask(IPv4Address());
        e->setGateway(lease->gateway);
        e->setInterface(ie);
        e->setSource(IPv4Route::MANUAL);
        irt->addRoute(e);
    }

    // update the routing table
    cancelEvent(leaseTimer);
    scheduleAt(simTime() + lease->leaseTime, leaseTimer);
}

void DHCPClient::unboundLease()
{
    EV_INFO << "Unbound lease on " << ie->getName() << "." << endl;

    cancelEvent(timerT1);
    cancelEvent(timerT2);
    cancelEvent(timerTo);
    cancelEvent(leaseTimer);

    ie->ipv4Data()->setIPAddress(IPv4Address());
    ie->ipv4Data()->setNetmask(IPv4Address::ALLONES_ADDRESS);
}

void DHCPClient::initClient()
{
    EV_INFO << "Starting DHCP configuration process." << endl;

    cancelEvent(timerT1);
    cancelEvent(timerT2);
    cancelEvent(timerTo);
    cancelEvent(leaseTimer);

    sendDiscover();
    scheduleTimerTO(WAIT_OFFER);
    clientState = SELECTING;
}

void DHCPClient::initRebootedClient()
{
     sendRequest();
     scheduleTimerTO(WAIT_ACK);
     clientState = REBOOTING;
}

void DHCPClient::handleDHCPMessage(DHCPMessage * msg)
{
    if (msg->getOp() != BOOTREPLY)
    {
        EV_WARN << "Client received a non-BOOTREPLY message. Drop it!" << endl;
        return;
    }

    if (msg->getXid() != xid)
    {
        EV_WARN << "Message transaction ID is not valid! Drop it!" << endl;
        return;
    }

    switch (clientState)
    {
        case SELECTING:
            if (msg->getOptions().getMessageType() == DHCPOFFER)
            {
                EV_INFO << "DHCPOFFER message arrived with IP address: " << msg->getYiaddr() << "." << endl;
                scheduleTimerTO(WAIT_ACK);
                clientState = REQUESTING;
                recordOffer(msg);
                sendRequest(); // we accept the first offer
            }
            else
                EV_WARN << "The arriving packet is not a DHCPOFFER. Drop it." << endl;
            break;
        case REQUESTING:
            if (msg->getOptions().getMessageType() == DHCPOFFER)
            {
                EV_WARN << "We don't accept DHCPOFFERs in REQUESTING state. Drop it." << endl; // remains in REQUESTING
            }
            else if (msg->getOptions().getMessageType() == DHCPACK)
            {
                recordLease(msg);
                EV_INFO << "The offered IP " << lease->ip << " is available. Assign it to " << this->getParentModule()->getFullName() << endl;
                cancelEvent(timerTo);
                scheduleTimerT1();
                scheduleTimerT2();
                boundLease();
                clientState = BOUND;
                /*
                    The client SHOULD perform a final check on the parameters (ping, ARP).
                    If the client detects that the address is already in use:
                    EV_INFO << "The offered IP " << lease->ip << " is not available." << endl;
                    sendDecline(lease->ip);
                    initClient();
                */
            }
            else if (msg->getOptions().getMessageType() == DHCPNAK)
            {
                EV_INFO << "Arrived DHCPNAK message. Restarting the configuration process." << endl;
                initClient();
            }
            break;
        case BOUND:
            EV_DETAIL << "We are in BOUND, discard all DHCP messages." << endl; // remain in BOUND
            break;
        case RENEWING:
            if (msg->getOptions().getMessageType() == DHCPACK)
            {
                EV_INFO << "Arrived DHCPACK message in RENEWING state." << endl;
                recordLease(msg);
                cancelEvent(timerTo);
                scheduleTimerT1();
                scheduleTimerT2();
                boundLease();
                clientState = BOUND;
            }
            if (msg->getOptions().getMessageType() == DHCPNAK)
            {
                unboundLease(); // halt network (remove address)
                EV_INFO << "The renewing process was unsuccessful. Restarting the DHCP configuration process." << endl;
                initClient();
            }
            break;
        case REBINDING:
            if (msg->getOptions().getMessageType() == DHCPNAK)
            {
                unboundLease(); // halt network (remove address)
                EV_INFO << "The rebinding process was unsuccessful. Restarting the DHCP configuration process." << endl;
                initClient();
            }
            if (msg->getOptions().getMessageType() == DHCPACK)
            {

                recordLease(msg);
                cancelEvent(timerTo);
                scheduleTimerT1();
                scheduleTimerT2();
                boundLease();
                clientState = BOUND;
            }
            break;
        case REBOOTING:
            if (msg->getOptions().getMessageType() == DHCPACK)
            {
                recordLease(msg);
                cancelEvent(timerTo);
                scheduleTimerT1();
                scheduleTimerT2();
                boundLease();
                clientState = BOUND;
            }
            if (msg->getOptions().getMessageType() == DHCPNAK)
                initClient();
            break;
        default:
            break;
    }
}

void DHCPClient::receiveChangeNotification(int category, const cPolymorphic *details)
{
    Enter_Method_Silent();
    printNotificationBanner(category, details);

    // host associated. link is up. change the state to init.
    if (category == NF_L2_ASSOCIATED)
    {
        InterfaceEntry * ie = NULL;
        if (details)
        {
            cPolymorphic *detailsAux = const_cast<cPolymorphic*>(details);
            ie = dynamic_cast<InterfaceEntry*>(detailsAux);
        }
        if (!ie || (ie && (ie == ie)))
        {
            EV_INFO << "Interface associated, starting DHCP." << endl;
            initClient();
        }
    }
    else if (category == NF_INTERFACE_DELETED)
    {
        if (isOperational)
            throw cRuntimeError("Reacting to interface deletions is not implemented in this module");
    }
}

void DHCPClient::sendRequest()
{
    // setting the xid
    xid = intuniform(0, RAND_MAX); // generating a new xid for each transmission

    DHCPMessage * request = new DHCPMessage("DHCPREQUEST");
    request->setOp(BOOTREQUEST);
    request->setByteLength(280); // DHCP request packet size
    request->setHtype(1); // ethernet
    request->setHlen(6); // hardware Address length (6 octets)
    request->setHops(0);
    request->setXid(xid); // transaction id
    request->setSecs(0); // 0 seconds from transaction started
    request->setBroadcast(false); // unicast
    request->setYiaddr(IPv4Address()); // no 'your IP' addr
    request->setGiaddr(IPv4Address()); // no DHCP Gateway Agents
    request->setChaddr(macAddress); // my mac address;
    request->setSname(""); // no server name given
    request->setFile(""); // no file given
    request->getOptions().setMessageType(DHCPREQUEST);
    request->getOptions().setClientIdentifier(macAddress);

    // set the parameters to request
    request->getOptions().setParameterRequestListArraySize(4);
    request->getOptions().setParameterRequestList(0,SUBNET_MASK);
    request->getOptions().setParameterRequestList(1,ROUTER);
    request->getOptions().setParameterRequestList(2,DNS);
    request->getOptions().setParameterRequestList(3,NTP_SRV);

    // RFC 4.3.6 Table 4
    if (clientState == INIT_REBOOT)
    {
        request->getOptions().setRequestedIp(lease->ip);
        request->setCiaddr(IPv4Address()); // zero
        EV_INFO << "Sending DHCPREQUEST asking for IP " << lease->ip << " via broadcast." << endl;
        sendToUDP(request, clientPort, IPv4Address::ALLONES_ADDRESS, serverPort);
    }
    else if (clientState == REQUESTING)
    {
        request->getOptions().setServerIdentifier(lease->serverId);
        request->getOptions().setRequestedIp(lease->ip);
        request->setCiaddr(IPv4Address()); // zero
        EV_INFO << "Sending DHCPREQUEST asking for IP " << lease->ip << " via broadcast." << endl;
        sendToUDP(request, clientPort, IPv4Address::ALLONES_ADDRESS, serverPort);
    }
    else if (clientState == RENEWING)
    {
        request->setCiaddr(lease->ip); // the client IP
        EV_INFO << "Sending DHCPREQUEST extending lease for IP " << lease->ip << " via unicast to " << lease->serverId << "." << endl;
        sendToUDP(request, clientPort, lease->serverId, serverPort);
    }
    else if (clientState == REBINDING)
    {
        request->setCiaddr(lease->ip); // the client IP
        EV_INFO << "Sending DHCPREQUEST renewing the IP " << lease->ip << " via broadcast." << endl;
        sendToUDP(request, clientPort, IPv4Address::ALLONES_ADDRESS, serverPort);
    }
}

void DHCPClient::sendDiscover()
{
    // setting the xid
    xid = intuniform(0, RAND_MAX);
    //std::cout << xid << endl;
    DHCPMessage* discover = new DHCPMessage("DHCPDISCOVER");
    discover->setOp(BOOTREQUEST);
    discover->setByteLength(280); // DHCP Discover packet size;
    discover->setHtype(1); // ethernet
    discover->setHlen(6); // hardware Address lenght (6 octets)
    discover->setHops(0);
    discover->setXid(xid); // transaction id
    discover->setSecs(0); // 0 seconds from transaction started
    discover->setBroadcast(false); // unicast
    discover->setChaddr(macAddress); // my mac address
    discover->setSname(""); // no server name given
    discover->setFile(""); // no file given
    discover->getOptions().setMessageType(DHCPDISCOVER);
    discover->getOptions().setClientIdentifier(macAddress);
    discover->getOptions().setRequestedIp(IPv4Address());

    // set the parameters to request
    discover->getOptions().setParameterRequestListArraySize(4);
    discover->getOptions().setParameterRequestList(0,SUBNET_MASK);
    discover->getOptions().setParameterRequestList(1,ROUTER);
    discover->getOptions().setParameterRequestList(2,DNS);
    discover->getOptions().setParameterRequestList(3,NTP_SRV);

    EV_INFO << "Sending DHCPDISCOVER." << endl;
    sendToUDP(discover, clientPort, IPv4Address::ALLONES_ADDRESS, serverPort);
}

void DHCPClient::sendDecline(IPv4Address declinedIp)
{
    xid = intuniform(0, RAND_MAX);
    DHCPMessage * decline = new DHCPMessage("DHCPDECLINE");
    decline->setOp(BOOTREQUEST);
    decline->setByteLength(280); // DHCPDECLINE packet size
    decline->setHtype(1); // ethernet
    decline->setHlen(6); // hardware Address length (6 octets)
    decline->setHops(0);
    decline->setXid(xid); // transaction id
    decline->setSecs(0); // 0 seconds from transaction started
    decline->setBroadcast(false); // unicast
    decline->setChaddr(macAddress); // my MAC address
    decline->setSname(""); // no server name given
    decline->setFile(""); // no file given
    decline->getOptions().setMessageType(DHCPDECLINE);
    decline->getOptions().setRequestedIp(declinedIp);
    EV_INFO << "Sending DHCPDECLINE." << endl;
    sendToUDP(decline, clientPort, IPv4Address::ALLONES_ADDRESS, serverPort);
}

void DHCPClient::scheduleTimerTO(TimerType type)
{
    // cancel the previous timeout
    cancelEvent(timerTo);
    timerTo->setKind(type);
    scheduleAt(simTime() + responseTimeout, timerTo);
}

void DHCPClient::scheduleTimerT1()
{
    // cancel the previous T1
    cancelEvent(timerT1);
    scheduleAt(simTime() + (lease->renewalTime), timerT1); // RFC 2131 4.4.5
}

void DHCPClient::scheduleTimerT2()
{
    // cancel the previous T2
    cancelEvent(timerT2);
    scheduleAt(simTime() + (lease->rebindTime), timerT2); // RFC 2131 4.4.5
}

void DHCPClient::sendToUDP(cPacket *msg, int srcPort, const IPvXAddress& destAddr, int destPort)
{
    EV_INFO << "Sending packet " << msg << endl;
    socket.sendTo(msg, destAddr, destPort, ie->getInterfaceId());
}

void DHCPClient::openSocket()
{
    socket.setOutputGate(gate("udpOut"));
    socket.bind(clientPort);
    socket.setBroadcast(true);
    EV_INFO << "DHCP server bound to port " << serverPort << "." << endl;
}

bool DHCPClient::handleOperationStage(LifecycleOperation *operation, int stage, IDoneCallback *doneCallback)
{
    Enter_Method_Silent();
    if (dynamic_cast<NodeStartOperation *>(operation))
    {
        if (stage == NodeStartOperation::STAGE_APPLICATION_LAYER)
        {
            isOperational = true;
            ie = chooseInterface();
            openSocket();
            if (lease != NULL)
            {
                clientState = INIT_REBOOT;
                initRebootedClient();
            }
            else // we have no lease from the previous DHCP process
            {
                clientState = INIT;
                initClient();
            }
        }
    }
    else if (dynamic_cast<NodeShutdownOperation *>(operation))
    {
        if (stage == NodeShutdownOperation::STAGE_APPLICATION_LAYER)
        {
            isOperational = false;
            cancelEvent(timerT1);
            cancelEvent(timerT2);
            cancelEvent(timerTo);
            cancelEvent(leaseTimer);
            cancelEvent(startTimer);
            // TODO: Client should send DHCPRELEASE to the server. However, the correct operation
            // of DHCP does not depend on the transmission of DHCPRELEASE messages.
            // TODO: socket.close();
            ie = NULL;
        }
    }
    else if (dynamic_cast<NodeCrashOperation *>(operation))
    {
        if (stage == NodeCrashOperation::STAGE_CRASH)
        {
            isOperational = false;
            cancelEvent(timerT1);
            cancelEvent(timerT2);
            cancelEvent(timerTo);
            cancelEvent(leaseTimer);
            cancelEvent(startTimer);
            ie = NULL;
        }
    }
    else
        throw cRuntimeError("Unsupported lifecycle operation '%s'", operation->getClassName());
    return true;
}

