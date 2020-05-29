//////////////////////////////////////////////////////////////////////////////
// This file is part of 'ADPgpCamlink'.
// It is subject to the license terms in the LICENSE.txt file found in the 
// top-level directory of this distribution and at: 
//    https://confluence.slac.stanford.edu/display/ppareg/LICENSE.html. 
// No part of 'ADPgpCamlink', including this file, 
// may be copied, modified, propagated, or distributed except according to 
// the terms contained in the LICENSE.txt file.
//////////////////////////////////////////////////////////////////////////////
//
//	pgpClSerialDev driver
//
//	device support using rogue PgpCamlink serial interface via CamLink
//

#include <stdio.h>
#include <unistd.h>
#include <iostream>
#include <DmaDriver.h>
#include <AxiVersion.h>

#include "pgpClSerialDev.h"
#include "rogue/GeneralError.h"

using namespace	std;

int	DEBUG_PGPCL_SER	= 2;

///	Constructor
pgpClSerialDev::pgpClSerialDev(
	unsigned int				board,
	unsigned int				lane )
:	m_board(	board		),
	m_lane(		lane		),
	m_devName(				),
	m_devLock(				)
{
	const char		*	functionName	= "pgpClSerialDev::pgpClSerialDev";

	if ( DEBUG_PGPCL_SER >= 1 )
		printf(  "%s constructor: board %u lane %u\n", functionName, board, lane );

	// Create mutexes
    m_devLock	= epicsMutexMustCreate();

	/*
	 * Check arguments
	 */

	char	acDevName[60];
	sprintf( acDevName, "/dev/datadev_%u", board );
	m_devName = acDevName;
	if ( DEBUG_PGPCL_SER >= 1 )
		printf(  "%s constructor: devName %s\n", functionName, m_devName.c_str() );

#if 0
	uint8_t mask[DMA_MASK_SIZE];
	dmaInitMaskBytes(mask);
	for (unsigned lane=0; lane<4; lane++) {
		dmaAddMaskBytes((uint8_t*)mask, (lane<<8 | channel));
	}
#endif

}

int pgpClSerialDev::connect( )
{
	uint32_t	dest	= 0;
	const char		*	functionName	= "pgpClSerialDev::connect";
	if ( DEBUG_PGPCL_SER >= 1 )
	{
		printf(  "%s: board %u lane %u\n", functionName, m_board, m_lane );
		sleep(2);
	}

	uint32_t ch = 2;	// Data channel 2 is used for pgpCamlink serial I/O
	dest		= (0x100 * m_lane) + ch;	// Derived from python code
	try
	{
		m_pDataChan	= rogue::hardware::axi::AxiStreamDma::create( m_devName, dest, true);
	}
	catch ( rogue::GeneralError & e )
	{
		printf(  "%s Error: Unable to create %s serial data channel!\n", functionName, m_devName.c_str() );
		disconnect();
		return -1;
	}

	try
	{
		// CHAN 2: Camera Serial Tx
		m_pClSerialTx = ClSerialMaster::create();
		if ( m_pClSerialTx && m_pDataChan )
		{
			m_pClSerialTx->addSlave( m_pDataChan );
			if ( DEBUG_PGPCL_SER >= 1 )
				printf(  "%s: Created lane %u ClSerialTx data channel\n", functionName, m_lane );
		}
		else
		{
			printf( "%s Error: Unable to connect %s ClSerialTx stream!\n", functionName, m_devName.c_str() );
		}
	}
	catch ( rogue::GeneralError & e )
	{
		printf( "%s Unable to connect %s ClSerialTx stream!\nRogue Error: %s\n", functionName, m_devName.c_str(), e.what() );
		disconnect();
		return -1;
	}

	try
	{
		// CHAN 3: Camera Serial Rx
		m_pClSerialRx = ClSerialSlave::create();
		if ( m_pClSerialRx && m_pDataChan )
		{
			m_pDataChan->addSlave( m_pClSerialRx );
			if ( DEBUG_PGPCL_SER >= 1 )
				printf(  "%s: Created lane %u ClSerialRx data channel\n", functionName, m_lane );
		}
		else
		{
			printf( "%s Unable to connect %s ClSerialRx stream!\n", functionName, m_devName.c_str() );
		}
	}
	catch ( rogue::GeneralError & e )
	{
		printf( "%s Unable to connect %s ClSerialRx stream!\nRogue Error: %s\n", functionName, m_devName.c_str(), e.what() );
		disconnect();
		return -1;
	}


#if 0
	// TODO: Move to pgpClSerialTool.cpp
	int fd = open(m_devName.c_str(), O_RDWR);
	if (fd < 0) {
		std::cout << "Error opening " << m_devName << std::endl;
	}
	else {
		AxiVersion vsn;
		if ( axiVersionGet(fd, &vsn) >= 0 )
		{
			printf("-- Core Axi Version --\n");
			printf("firmwareVersion : %x\n", vsn.firmwareVersion);
			printf("buildString     : %s\n", vsn.buildString); 
		}
		close( fd );
	}
#endif

	if ( DEBUG_PGPCL_SER >= 1 )
		printf(  "%s: Connected lane %u serial data channels\n", functionName, m_lane );
	return 0;
}

void pgpClSerialDev::disconnect( )
{
	const char		*	functionName	= "pgpClSerialDev::disconnect";
	if ( DEBUG_PGPCL_SER >= 1 )
		printf(  "%s: board %u lane %u\n", functionName, m_board, m_lane );

	// Release the rogue interfaces and data channel
	m_pClSerialRx.reset();
	m_pClSerialTx.reset();
	m_pDataChan.reset();
}

/// virtual Destructor
pgpClSerialDev::~pgpClSerialDev()
{
	disconnect();
}

int pgpClSerialDev::sendBytes( const unsigned char * buffer, size_t nBytes )
{
	const char		*	functionName	= "pgpClSerialDev::sendBytes";
	if ( ! m_pClSerialTx )
	{
		printf(  "%s: board %u lane %u, invalid ClSerialTx data channel!\n", functionName, m_board, m_lane );
		return -1;
	}

	if ( DEBUG_PGPCL_SER >= 1 )
		printf(  "%s: board %u lane %u, sending %zu bytes\n", functionName, m_board, m_lane, nBytes );
	return m_pClSerialTx->sendBytes( buffer, nBytes );
}

int pgpClSerialDev::readBytes( unsigned char * buffer, double timeout, size_t nBytes )
{
	const char		*	functionName	= "pgpClSerialDev::readBytes";
	if ( ! m_pClSerialRx )
	{
		printf(  "%s: board %u lane %u, invalid ClSerialRx data channel!\n", functionName, m_board, m_lane );
		return -1;
	}

	if ( DEBUG_PGPCL_SER >= 1 )
		printf(  "%s: board %u lane %u\n", functionName, m_board, m_lane );
	int		nRead = m_pClSerialRx->readBytes( buffer, timeout, nBytes );
	if ( nRead <= 0 )
	{
		if ( DEBUG_PGPCL_SER >= 1 )
			printf(  "%s: board %u lane %u read error!\n", functionName, m_board, m_lane );
		return 0;
	}
	return nRead;
}
