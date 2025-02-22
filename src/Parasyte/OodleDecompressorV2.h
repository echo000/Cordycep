#pragma once
#include "Oodle.h"
#include "Decompressor.h"

namespace ps
{
	// A class to handle V2 Oodle Fast Files.
	class OodleDecompressorV2 : public Decompressor
	{
	private:
		// The current block index.
		size_t BlockIndex;
		// The next hash block index;
		size_t NextHashBlockIndex;
	public:
		// Creates a new Oodle V2 decompressor.
		OodleDecompressorV2(ps::FileHandle& file, bool secure);

		// Decompress data.
		size_t Decompress(uint8_t* input, size_t inputSize, uint8_t* output, size_t outputSize);
		// Reads decompressed data from the data stream.
		size_t Read(void* ptr, const size_t size, const size_t offset);
	};
}

