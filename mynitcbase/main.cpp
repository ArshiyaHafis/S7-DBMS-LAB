#include "Buffer/StaticBuffer.h"
#include "Cache/OpenRelTable.h"
#include "Disk_Class/Disk.h"
#include "FrontendInterface/FrontendInterface.h"
#include <iostream>

int main(int argc, char *argv[]) {
	Disk disk_run;

	unsigned char buffer[BLOCK_SIZE];
	char message[2048];
	Disk::readBlock(buffer, 0);
	for (int i = 0; i < 2048; ++i) {
		std::cout << static_cast<int>(buffer[i]) << std::endl;
	}

	Disk::readBlock(buffer, 1);
	for (int i = 0; i < 2048; ++i) {
		std::cout << static_cast<int>(buffer[i]) << std::endl;
	}
	
	Disk::readBlock(buffer, 2);
	for (int i = 0; i < 2048; ++i) {
		std::cout << static_cast<int>(buffer[i]) << std::endl;
	}
	
	Disk::readBlock(buffer, 3);
	for (int i = 0; i < 2048; ++i) {
		std::cout << static_cast<int>(buffer[i]) << std::endl;
	}

	return 0;
}

// int main(int argc, char *argv[]) {
//   /* Initialize the Run Copy of Disk */
//   Disk disk_run;
//   // StaticBuffer buffer;
//   // OpenRelTable cache;

//   return FrontendInterface::handleFrontend(argc, argv);
// }