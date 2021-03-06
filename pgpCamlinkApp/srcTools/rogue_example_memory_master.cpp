#include <rogue/interfaces/memory/Constants.h>
#include <rogue/interfaces/memory/Master.h>
#include <rogue/hardware/axi/AxiMemMap.h>

namespace rh	= rogue::hardware;
namespace rim	= rogue::interfaces::memory;

// Create a subclass of a memory Master
// This master will initiate a read from a passed adddress and
// increment that value at that address by a passed value
class PgpMemMaster : public rogue::interfaces::memory::Master
{
	public:

	// Create a static class creator to return our custom class
	// wrapped with a shared pointer
	static std::shared_ptr<PgpMemMaster> create()
	{
		static std::shared_ptr<PgpMemMaster> ret =
		std::make_shared<PgpMemMaster>();
		return(ret);
	}

	// Standard class creator which is called by create
	PgpMemMaster() : rogue::interfaces::memory::Master() {}

	// Start a sequence with passed address and increment value
	bool incAtAddress(uint64_t address, uint32_t value)
	{
		uint32_t rValue;
		uint32_t id;

		// Clear any existing errors
		//this->setError(0);

		// Start read transaction, size=4
		// uint32_t reqTransaction(uint64_t address, uint32_t size, void *data, uint32_t type);
		// types from rogue/interfaces/memory/Constants.h:
		//		rim::Read, rim::Post, rim::Write, rim::Verify
		id = this->reqTransaction(address, 4, &rValue, rogue::interfaces::memory::Read);

		// Wait for transaction to complete
		this->waitTransaction(id);

		// Check transaction result
		if ( this->getError() != "" )
		{
			printf("got error\n");
			return false;
		}

		// Increment value by passed value
		rValue += value;

		// Start write transaction, size = 4
		id = this->reqTransaction(address, 4, &rValue, rogue::interfaces::memory::Write);

		// Wait for transaction to complete
		this->waitTransaction(id);

		// Check transaction result
		if ( this->getError() != "" )
		{
			printf("got error\n");
			return false;
		}
		else
			return true;
	}
};

// Shared pointer alias
typedef std::shared_ptr<PgpMemMaster> PgpMemMasterPtr;
