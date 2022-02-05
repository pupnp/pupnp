// Tools and helper classes to manage gtests
// Author: 2021-03-12 - Ingo Höft

#include <ifaddrs.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <iostream>
#include <fstream>
#include <fcntl.h>


class CIfaddr4
// Tool to manage and fill a socket address structure. This is needed
// for mocked network interfaces.
{
	struct sockaddr_in ifa_addr;	// network address
	struct sockaddr_in ifa_netmask;	// netmask
	struct sockaddr_in ifa_ifu;		// broadcast addr or point-to-point dest addr
	struct ifaddrs ifaddr;

	// the bitmask is the offset in the netmasks array.
	std::string netmasks[33] = {"0.0.0.0",
			"128.0.0.0", "192.0.0.0", "224.0.0.0", "240.0.0.0",
			"248.0.0.0", "252.0.0.0", "254.0.0.0", "255.0.0.0",
			"255.128.0.0", "255.192.0.0", "255.224.0.0", "255.240.0.0",
			"255.248.0.0", "255.252.0.0", "255.254.0.0", "255.255.0.0",
			"255.255.128.0", "255.255.192.0", "255.255.224.0", "255.255.240.0",
			"255.255.248.0", "255.255.252.0", "255.255.254.0", "255.255.255.0",
			"255.255.255.128", "255.255.255.192", "255.255.255.224", "255.255.255.240",
			"255.255.255.248", "255.255.255.252", "255.255.255.254", "255.255.255.255"};
public:
	CIfaddr4()
	// With constructing the object you get a loopback device by default.
	{
		// loopback interface
		//-------------------
		// set network address
		ifa_addr.sin_family = AF_INET;
		//ifa_addr.sin_port = htons(MYPORT);
		inet_aton("127.0.0.1", &(ifa_addr.sin_addr));

		// set netmask
		ifa_netmask.sin_family = AF_INET;
		//ifa_netmask.sin_port = htons(MYPORT);
		inet_aton("255.0.0.0", &(ifa_netmask.sin_addr));

		// set broadcast address or Point-to-point destination address
		ifa_ifu.sin_family = AF_INET;
		//ifa_ifu.sin_port = htons(MYPORT);
		inet_aton("0.0.0.0", &(ifa_ifu.sin_addr));

		ifaddr.ifa_next = nullptr; 	// pointer to next ifaddrs structure
		ifaddr.ifa_name = (char*)"lo";
		// v-- Flags from SIOCGIFFLAGS, man 7 netdevice
		ifaddr.ifa_flags = 0 | IFF_LOOPBACK | IFF_UP;
		ifaddr.ifa_addr = (struct sockaddr*)&ifa_addr;
		ifaddr.ifa_netmask = (struct sockaddr*)&ifa_netmask;
		ifaddr.ifa_broadaddr = (struct sockaddr*)&ifa_ifu;
		ifaddr.ifa_data = nullptr;
	}


	ifaddrs* get()
	// Return the pointer to the ifaddr structure
	{
		return &ifaddr;
	}


	bool set(std::string pIfname, std::string pIfaddress)
	// Set the interface name and the ipv4 address with bitmask. Properties are
	// set to an ipv4 UP interface, supporting broadcast and multicast.
	// Returns true if successful.
	{
		ifaddr.ifa_name = (char*)pIfname.c_str();

		// get the netmask from the bitmask
		// the bitmask is the offset in the netmasks array.
		std::size_t slashpos = pIfaddress.find_first_of("/");
		std::string address = pIfaddress;
		std::string bitmask = "32";
		if (slashpos != std::string::npos) {
			address = pIfaddress.substr(0,slashpos);
			bitmask = pIfaddress.substr(slashpos+1);
		}
		//std::cout << "address: '" << address << "', bitmask: '" << bitmask << "', netmask: "
		//			<< netmasks[std::stoi(bitmask)] << ", slashpos: " << slashpos << "\n";

		// convert address strings to numbers and store them
		inet_aton(address.c_str(), &(ifa_addr.sin_addr));
		std::string netmask = netmasks[std::stoi(bitmask)];
		inet_aton(netmask.c_str(), &(ifa_netmask.sin_addr));
		ifaddr.ifa_flags = 0 | IFF_UP | IFF_BROADCAST | IFF_MULTICAST;

		// calculate broadcast address as follows: broadcast = ip | ( ~ subnet )
		// broadcast = ip-addr or the inverted subnet-mask
		ifa_ifu.sin_addr.s_addr = ifa_addr.sin_addr.s_addr | ~ ifa_netmask.sin_addr.s_addr;

		return true;
	}
};

class CCaptureFd
// Tool to capture output to a file descriptor, mainly used to capture program
// output to stdout or stderr.
// When printing the captured output, all opened file descriptor will be closed
// to avoid confusing output loops. For a new capture after print(..) you have
// to call capture(..) again.
{
	int mFd;
	int mFd_old;
	int mFd_log;
	bool mErr = true;
	char mCaptFname[16] = ".captfd.log";

public:
	void capture(int fd)
	{
		mFd = fd;
		mFd_old = dup(fd);
		if (mFd_old < 0) {
			return;
		}
		mFd_log = ::open(mCaptFname, O_WRONLY|O_CREAT|O_TRUNC, 0660);
		if (mFd_log < 0) {
			::close(mFd_old);
			return;
		}
		if (dup2(mFd_log, fd) < 0) {
			::close(mFd_old);
			::close(mFd_log);
			return;
		}
		mErr = false;
	}

	~CCaptureFd()
	{
		this->closeFds();
		remove(mCaptFname);
	}

	bool print(std::ostream &pOut)
	// Close all file descriptors and print captured file content.
	// If nothing was captured, then the return value is false.
	{
		if (mErr) return false;
		this->closeFds();

		std::ifstream readFileObj(mCaptFname);
		std::string lineBuf = "";

		std::getline(readFileObj, lineBuf);
		if (lineBuf == "") {
			readFileObj.close();
			remove(mCaptFname);
			return false;
		}

		pOut << lineBuf << "\n";
		while (std::getline(readFileObj, lineBuf))
				pOut << lineBuf << "\n";

		readFileObj.close();
		remove(mCaptFname);
		return true;
	}

private:
	void closeFds()
	{
		// restore old fd
		dup2(mFd_old, mFd);
		::close(mFd_old);
		::close(mFd_log);
		mErr = true;
	}
};
