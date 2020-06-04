#include <string>
#include <string.h>
#include "ImageStream.h"
#include "pgpCamlink.h"
#include <rogue/interfaces/stream/Frame.h>
#include <rogue/interfaces/stream/FrameIterator.h>

extern int	DEBUG_PGP_CAMLINK;

void ImageStream::acceptFrame ( rogue::interfaces::stream::FramePtr frame )
{
	const char	*	functionName	= "ImageStream::acceptFrame";
	if ( !frame ) {
		if ( DEBUG_PGP_CAMLINK >= 2 )
			printf( "%s: No frame!\n", functionName );
		return;
	}
	// Above test not sufficient to avoid:
	// terminate called after throwing an instance of 'std::bad_weak_ptr'
	//   what():  bad_weak_ptr
	//   Aborted (core dumped)

	// Acquire lock on frame. Will be release when lock class goes out of scope
	rogue::interfaces::stream::FrameLockPtr lock = frame->lock();

	// Here we get an iterator to the frame data
	rogue::interfaces::stream::FrameIterator it;
	it = frame->begin();

	if ( DEBUG_PGP_CAMLINK >= 4 )
	{
		printf( "\n" );
		// Print the values in the first 10 locations
		printf( "ImageStream::acceptFrame: SuperFrameSize=%u bytes:", frame->getPayload() );
#if 0
		for ( uint32_t x=0; x < 20; x++)
		{
#if 0
			uint16_t	pixelData;
			fromFrame( it, 2, &pixelData );
			printf( " 0x%04x", pixelData );
#else
			printf( " 0x%02x", *it );
			it++;
#endif
		}
#endif
		printf( " ...\n" );
	}

	// Use std::copy to copy data to a data buffer
	// Here we copy the entire frame payload to the data buffer
	// std::copy(frame->begin(), frame->end(), data);

	// Timestamp should default to TOD
	epicsTimeStamp		ts;
	epicsTimeGetCurrent( &ts );

#if 0
	// Process frame via CoreV1 protocol
	rogue::protocols::batcher::CoreV1		core;
	core.processFrame(frame);
	if ( DEBUG_PGP_CAMLINK >= 4 )
		printf( "ImageStream::acceptFrame: core count=%u, seq=%u, hdrSize=%u, tailSize=%u\n",
				core.count(), core.sequence(), core.headerSize(), core.tailSize() );
	for ( uint32_t sf = 0; sf < core.count(); sf++ )
	{
		rogue::protocols::batcher::DataPtr	data;
		data = core.record(sf);
		// FUSER_BIT_1 = StartOfFrame
		// LUSER_BIT_0 = FrameError
		if ( DEBUG_PGP_CAMLINK >= 4 )
			printf( "ImageStream::acceptFrame SubFrame %d: dest=%u, size=%u, fUser=0x%02x, lUser=0x%02x\n",
					sf, data->dest(), data->size(), data->fUser(), data->lUser() );
		if ( data->dest() == 0 )
		{	// TDEST 0 is Timing Event
			// Offset 0:  4 byte nsec of timestamp
			// Offset 4:  4 byte sec  of timestamp
			// Offset 8:  4 byte edefAvgDoneMask
			// Offset 12: 4 byte edefAvgMinorMask
			// Offset 16: 4 byte edefAvgMajorMask
			// Offset 20: 4 byte edefInitMask
			// Offset 24: 4 byte Modifier1
			// Offset 28: 4 byte Modifier2
			// Offset 32: 4 byte Modifier3
			// Offset 36: 4 byte Modifier4
			// Offset 40: 4 byte Modifier5
			// Offset 44: 4 byte Modifier6
			it = data->begin();
			fromFrame( it, 4, &ts.nsec );
			fromFrame( it, 4, &ts.secPastEpoch );
			char        acBuff[40];
			epicsTimeToStrftime( acBuff, 40, "%H:%M:%S.%04f", &ts );
			if ( DEBUG_PGP_CAMLINK >= 4 )
			{
				printf( "ts %s, pulseId 0x%X\n", acBuff, ts.nsec & 0x1FFFF );
				if ( DEBUG_PGP_CAMLINK >= 5 )
				{
					printf( "Invalid timing frame:" );
					for ( uint32_t x=0; x < 24; x++)
					{
						printf( " 0x%02x", *it );
						it++;
					}
					printf( "\n" );
				}
			}
		}
		else if ( data->dest() == 1 && 0 )
		{	// TDEST 1 is framegrabber image data
			//printf( "ImageStream::acceptFrame SubFrame %d: ", sf );
			//it = data->begin();
			it = data->end();
			for ( uint32_t x=0; x < 1030; x++)
			{
#if 1
				if ( (x % 16) == 0 )
					printf( "\n" );
				uint16_t	pixelData;
				it -= 2;
				fromFrame( it, 2, &pixelData );
				printf( " 0x%04x", pixelData );
				it -= 2;
#else
				printf( " 0x%02x", *it );
				it++;
#endif
			}
			printf( "\n" );
		}
	}
#endif

	// Report the new frame to the camera device
//	if( m_pCamDev )
//		m_pCamDev->NewImage( frame, ts );
}
