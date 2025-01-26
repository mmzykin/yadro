#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/lte-module.h"
#include "ns3/mobility-module.h"
#include "ns3/config-store.h"
#include <iostream>
#include <fstream>
#include <string>
#include <map>
using namespace ns3;

void CalculateThroughput() {
    std::ifstream traceFile("rlc-stats.txt");
    if (!traceFile.is_open()) {
        std::cerr << "Error: Unable to open RLC trace file." << std::endl;
        return;
    }

    std::map<uint64_t, uint64_t> dlBytes; 
    std::map<uint64_t, uint64_t> ulBytes; 

    std::string line;
    while (std::getline(traceFile, line)) {
        if (line.find("DL") != std::string::npos) {
            uint64_t userId, bytes;
            if (sscanf(line.c_str(), "%*f %lu %*s %*s %lu", &userId, &bytes) == 2) {
                dlBytes[userId] += bytes;
            }
        }
        else if (line.find("UL") != std::string::npos) {
            uint64_t userId, bytes;
            if (sscanf(line.c_str(), "%*f %lu %*s %*s %lu", &userId, &bytes) == 2) {
                ulBytes[userId] += bytes;
            }
        }
    }

    traceFile.close();

    double simulationTime = 0.01; 
    for (const auto& entry : dlBytes) {
        uint64_t userId = entry.first;
        double dlThroughput = (entry.second * 8) / (simulationTime * 1e6); 
        double ulThroughput = (ulBytes[userId] * 8) / (simulationTime * 1e6); 

        std::cout << "User " << userId << " - DL Throughput: " << dlThroughput << " Mbps, "
                  << "UL Throughput: " << ulThroughput << " Mbps" << std::endl;
    }
}

int main(int argc, char *argv[]) {
    NodeContainer baseStations;
    baseStations.Create(1);

    NodeContainer userDevices;
    userDevices.Create(2);

    Ptr<LteHelper> lteHelper = CreateObject<LteHelper>();

    MobilityHelper mobilitySetup;
    mobilitySetup.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobilitySetup.Install(baseStations);
    mobilitySetup.Install(userDevices);

    NetDeviceContainer baseStationDevices = lteHelper->InstallEnbDevice(baseStations);
    NetDeviceContainer userDeviceDevices = lteHelper->InstallUeDevice(userDevices);

    lteHelper->Attach(userDeviceDevices, baseStationDevices.Get(0));

    EpsBearer::Qci qosClass = EpsBearer::GBR_CONV_VOICE;
    EpsBearer qosProfile(qosClass);
    lteHelper->ActivateDataRadioBearer(userDeviceDevices, qosProfile);

    lteHelper->SetSchedulerType("ns3::PfFfMacScheduler");

    lteHelper->EnableRlcTraces();

    Simulator::Stop(Seconds(0.01)); 
    Simulator::Run();
    Simulator::Destroy();

    CalculateThroughput();

    return 0;
}
