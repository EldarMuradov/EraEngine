// Copyright (c) 2023-present Eldar Muradov. All rights reserved.

#include "asset/deflate.h"

namespace era_engine
{
	struct BitStream
	{
		template <typename T>
		T* consume(uint32 count = 1)
		{
			T* result = (T*)(data + read_offset);
			read_offset += sizeof(T) * count;
			ASSERT(read_offset <= size);
			return result;
		}

		uint32 peek_bits(uint32 _bit_count)
		{
			ASSERT(_bit_count <= 32);

			uint32 result = 0;

			while ((this->bit_count < _bit_count))
			{
				uint32 byte = *consume<uint8>();
				this->bit_buffer |= (byte << this->bit_count);
				this->bit_count += 8;
			}

			result = this->bit_buffer & ((1 << _bit_count) - 1);

			return result;
		}

		void discard_bits(uint32 _bit_count)
		{
			bit_count -= _bit_count;
			bit_buffer >>= _bit_count;
		}

		uint32 consume_bits(uint32 _bit_count)
		{
			uint32 result = peek_bits(_bit_count);
			discard_bits(_bit_count);
			return result;
		}

		void flush_byte()
		{
			uint32 flushCount = (bit_count % 8);
			consume_bits(flushCount);
		}

		uint64 bytes_remaining() const
		{
			return size - read_offset;
		}

		uint8* data;
		uint64 size;
		uint64 read_offset;

		uint32 bit_count;
		uint32 bit_buffer;
	};

	inline constexpr uint32 reverse_bits(uint32 value, uint32 bit_count)
	{
		uint32 result = 0;

		for (uint32 i = 0; i <= (bit_count / 2); ++i)
		{
			uint32 inv = (bit_count - (i + 1));
			result |= ((value >> i) & 0x1) << inv;
			result |= ((value >> inv) & 0x1) << i;
		}

		return result;
	}

	struct HuffmanEntry
	{
		uint16 symbol;
		uint16 code_length;
	};

	struct HuffmanTable
	{
		uint32 max_code_length_in_bits;
		std::vector<HuffmanEntry> entries;

		void initialize(uint32 _max_code_length_in_bits, uint32* symbol_lengths, uint32 num_symbols, uint32 symbol_offset = 0)
		{
			ASSERT(_max_code_length_in_bits <= 16);

			max_code_length_in_bits = _max_code_length_in_bits;
			uint32 entryCount = (1 << max_code_length_in_bits);
			entries.resize(entryCount);

			uint32 bl_count[16] = {};
			for (uint32 i = 0; i < num_symbols; ++i)
			{
				uint32 count = symbol_lengths[i];
				ASSERT(count < arraysize(bl_count));
				++bl_count[count];
			}

			uint32 next_code[16];
			next_code[0] = 0;
			bl_count[0] = 0;
			for (uint32 bits = 1; bits < 16; ++bits)
			{
				next_code[bits] = ((next_code[bits - 1] + bl_count[bits - 1]) << 1);
			}

			for (uint32 n = 0; n < num_symbols; ++n)
			{
				uint32 len = symbol_lengths[n];
				if (len)
				{
					uint32 code = next_code[len]++;

					uint32 num_garbage_bits = max_code_length_in_bits - len;
					uint32 num_entries = (1 << num_garbage_bits);

					for (uint32 i = 0; i < num_entries; ++i)
					{
						uint32 base = (code << num_garbage_bits) | i;
						uint32 index = reverse_bits(base, max_code_length_in_bits);

						HuffmanEntry& entry = entries[index];

						uint32 symbol = n + symbol_offset;
						entry.code_length = (uint16)len;
						entry.symbol = (uint16)symbol;
					}
				}
			}
		}

		uint32 decode(BitStream& stream)
		{
			uint32 index = stream.peek_bits(max_code_length_in_bits);
			HuffmanEntry entry = entries[index];
			stream.discard_bits(entry.code_length);
			return entry.symbol;
		}
	};

	static HuffmanEntry length_extra[] =
	{
		{3, 0}, {4, 0}, {5, 0}, {6, 0}, {7, 0}, {8, 0}, {9, 0}, {10, 0}, {11, 1}, {13, 1}, {15, 1}, {17, 1}, {19, 2},
		{23, 2}, {27, 2}, {31, 2}, {35, 3}, {43, 3}, {51, 3}, {59, 3}, {67, 4}, {83, 4}, {99, 4}, {115, 4}, {131, 5},
		{163, 5}, {195, 5}, {227, 5}, {258, 0}
	};

	static HuffmanEntry dist_extra[] =
	{
		{1, 0}, {2, 0}, {3, 0}, {4, 0}, {5, 1}, {7, 1}, {9, 2}, {13, 2}, {17, 3}, {25, 3}, {33, 4}, {49, 4}, {65, 5},
		{97, 5}, {129, 6}, {193, 6}, {257, 7}, {385, 7}, {513, 8}, {769, 8}, {1025, 9}, {1537, 9}, {2049, 10}, {3073, 10},
		{4097, 11}, {6145, 11}, {8193, 12}, {12289, 12}, {16385, 13}, {24577, 13}
	};


	uint64 decompress(uint8* data, uint64 compressed_size, uint8* output)
	{
		BitStream stream = { data, compressed_size };

		uint32 zlibHeader0 = *stream.consume<uint8>();
		uint32 zlibHeader1 = *stream.consume<uint8>();
		uint32 counter = (((zlibHeader0 * 256 + zlibHeader1) % 31 != 0) || (zlibHeader1 & 32) || ((zlibHeader0 & 15) != 8));
		ASSERT(counter == 0);

		uint8* output_start = output;

		uint32 BFINAL = 0;
		while (BFINAL == 0)
		{
			BFINAL = stream.consume_bits(1);
			uint32 BTYPE = stream.consume_bits(2);

			ASSERT(BTYPE != 3);

			if (BTYPE == 0)
			{
				// No compression
				stream.flush_byte();

				uint16 LEN = (uint16)stream.consume_bits(16);
				uint16 NLEN = (uint16)stream.consume_bits(16);
				ASSERT((uint16)LEN == (uint16)~NLEN);

				while (LEN)
				{
					uint16 useLEN = LEN;
					useLEN = (uint16)min((uint64)useLEN, stream.bytes_remaining());

					uint8* source = stream.consume<uint8>(useLEN);
					uint16 copy_count = useLEN;
					while (copy_count--)
					{
						*output++ = *source++;
					}

					LEN -= useLEN;
				}
			}
			else
			{
				uint32 litlen_dist[512];

				uint32 HLIT = 0;
				uint32 HDIST = 0;
				if (BTYPE == 2)
				{
					// Compressed with dynamic Huffman codes.
					HLIT = stream.consume_bits(5) + 257;
					HDIST = stream.consume_bits(5) + 1;
					uint32 HCLEN = stream.consume_bits(4) + 4;

					uint32 HCLENSwizzle[] = { 16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15 };
					ASSERT(HCLEN <= arraysize(HCLENSwizzle));
					uint32 HCLENTable[arraysize(HCLENSwizzle)] = {};

					for (uint32 i = 0; i < HCLEN; ++i)
					{
						HCLENTable[HCLENSwizzle[i]] = stream.consume_bits(3);
					}

					HuffmanTable dict_table;
					dict_table.initialize(7, HCLENTable, arraysize(HCLENSwizzle));

					uint32 out_index = 0;
					while (out_index < HLIT + HDIST)
					{
						uint32 len = dict_table.decode(stream);
						uint32 value = 0;
						uint32 repeat = 1;
						if (len < 16)
						{
							value = len;
						}
						else if (len == 16)
						{
							repeat = stream.consume_bits(2) + 3;
							ASSERT(out_index > 0);
							value = litlen_dist[out_index - 1];
						}
						else if (len == 17)
						{
							repeat = stream.consume_bits(3) + 3;
						}
						else if (len == 18)
						{
							repeat = stream.consume_bits(7) + 11;
						}

						for (uint32 r = 0; r < repeat; ++r)
						{
							litlen_dist[out_index++] = value;
						}
					}
					ASSERT(out_index == HLIT + HDIST);
				}
				else if (BTYPE == 1)
				{
					HLIT = 288;
					HDIST = 32;

					uint32 bit_counts[][2] = { {143, 8}, {255, 9}, {279, 7}, {287, 8}, {319, 5} };

					uint32 out_index = 0;
					for (uint32 i = 0; i < arraysize(bit_counts); ++i)
					{
						uint32 last_value = bit_counts[i][0];
						uint32 bit_count = bit_counts[i][1];
						while (out_index <= last_value)
						{
							litlen_dist[out_index++] = bit_count;
						}
					}
				}

				HuffmanTable lit_len_table;
				lit_len_table.initialize(15, litlen_dist, HLIT);

				HuffmanTable dist_table;
				dist_table.initialize(15, litlen_dist + HLIT, HDIST);

				while (true)
				{
					uint32 lit_len = lit_len_table.decode(stream);
					if (lit_len <= 255)
					{
						*output++ = lit_len;
					}
					else if (lit_len >= 257)
					{
						HuffmanEntry len_entry = length_extra[lit_len - 257];
						uint32 len = len_entry.symbol;
						if (len_entry.code_length)
						{
							len += stream.consume_bits(len_entry.code_length);
						}

						uint32 dist_index = dist_table.decode(stream);
						HuffmanEntry dist_entry = dist_extra[dist_index];
						uint32 dist = dist_entry.symbol;
						if (dist_entry.code_length)
						{
							dist += stream.consume_bits(dist_entry.code_length);
						}

						uint8* input = output - dist;
						for (uint32 r = 0; r < len; ++r)
						{
							*output++ = *input++;
						}
					}
					else
						break;
				}
			}
		}

		return output - output_start;
	}
}