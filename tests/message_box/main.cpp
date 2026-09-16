#include "RE/Starfield.h"

#include <cstdlib>
#include <iostream>
#include <unordered_set>

namespace
{
	int                       managerStorage;
	void*                     manager = &managerStorage;
	std::unordered_set<void*> allocations;
	std::string               expected;
	bool                      consume = true;
	std::size_t               creates = 0;
	std::size_t               destroys = 0;
	std::size_t               assignments = 0;

	void Check(bool a_condition, const char* a_message)
	{
		if (!a_condition) {
			std::cerr << a_message << '\n';
			std::exit(1);
		}
	}

	template <class T>
	T Read(const void* a_data, std::size_t a_offset)
	{
		T value;
		std::memcpy(&value, static_cast<const std::byte*>(a_data) + a_offset, sizeof(T));
		return value;
	}

	template <class T>
	void Write(void* a_data, std::size_t a_offset, T a_value)
	{
		std::memcpy(static_cast<std::byte*>(a_data) + a_offset, &a_value, sizeof(T));
	}

	const char* StringData(const void* a_string)
	{
		return Read<std::uint16_t>(a_string, 0xC) > 12 ? Read<const char*>(a_string, 0) : static_cast<const char*>(a_string);
	}

	// Independent stand-ins for the engine ABI observed in Starfield 1.16.244.
	bool Assign(void* a_string, const char* a_text, std::size_t a_length)
	{
		++assignments;
		Check(a_length == 0, "String assignment must use the complete NUL-terminated input");
		Check(Read<std::uint16_t>(a_string, 0xC) == 1, "String capacity must start at one");
		Check(Read<std::uint16_t>(a_string, 0xE) == 0, "String length must start at zero");
		const auto text = a_text ? a_text : "";
		const auto size = std::strlen(text);
		char*      buffer = static_cast<char*>(a_string);
		if (size + 1 > 12) {
			buffer = new char[size + 1];
			allocations.insert(buffer);
			Write(a_string, 0, buffer);
		}
		std::memcpy(buffer, text, size + 1);
		Write(a_string, 0xC, static_cast<std::uint16_t>(size + 1));
		Write(a_string, 0xE, static_cast<std::uint16_t>(size));
		return size != 0;
	}

	void ReleaseString(void* a_string)
	{
		if (Read<std::uint16_t>(a_string, 0xC) > 12) {
			auto buffer = Read<char*>(a_string, 0);
			Check(allocations.erase(buffer) == 1, "Invalid or repeated string release");
			delete[] buffer;
		}
		std::memset(a_string, 0, 16);
		Write(a_string, 0xC, std::uint16_t{ 1 });
	}

	void Destroy(void* a_request)
	{
		++destroys;
		Check(Read<void*>(a_request, 0) == nullptr, "Debug message must not install a callback");
		for (const auto offset : { 0x28, 0x18, 0x08 }) {
			ReleaseString(static_cast<std::byte*>(a_request) + offset);
		}
	}

	void Create(void* a_manager, void* a_request, bool a_ensureUnique)
	{
		++creates;
		Check(a_manager == &managerStorage, "Wrong manager pointer");
		Check(!a_ensureUnique, "Debug.MessageBox permits repeated messages");
		Check(Read<void*>(a_request, 0) == nullptr, "Unexpected callback");
		Check(Read<std::uint32_t>(a_request, 0x38) == 5, "Wrong script warning category");
		Check(std::string_view(StringData(static_cast<std::byte*>(a_request) + 0x08)) == "DEBUG", "Wrong title or string layout");
		Check(std::string_view(StringData(static_cast<std::byte*>(a_request) + 0x18)) == expected, "Message differs from caller input");
		Check(std::string_view(StringData(static_cast<std::byte*>(a_request) + 0x28)).empty(), "Third string must be empty");
		if (consume) {
			// The native Create moves these buffers; simulate ownership leaving the request.
			for (const auto offset : { 0x08, 0x18, 0x28 }) {
				ReleaseString(static_cast<std::byte*>(a_request) + offset);
			}
		}
	}
}

// Link-time substitution of the address database only. The test calls the real
// library implementation and real REL::Relocation; no game or versionlib is loaded.
namespace REL
{
	IDDB::IDDB() = default;

	std::uint64_t IDDB::offset(std::uint64_t a_id) const
	{
		std::uintptr_t address = 0;
		switch (a_id) {
		case 36345:
			address = reinterpret_cast<std::uintptr_t>(&Assign);
			break;
		case 43998:
			address = reinterpret_cast<std::uintptr_t>(&Destroy);
			break;
		case 114231:
			address = reinterpret_cast<std::uintptr_t>(&Create);
			break;
		case 938019:
			address = reinterpret_cast<std::uintptr_t>(&manager);
			break;
		default:
			Check(false, "Unexpected relocation: the Papyrus callback must not be used");
		}
		return address - REX::FModule::GetExecutingModule().GetBaseAddress();
	}
}

int main()
{
	manager = nullptr;
	RE::DebugMessageBox("Not ready");
	Check(creates == 0 && destroys == 0 && assignments == 0, "Missing manager should do nothing");
	manager = &managerStorage;
	const std::string messages[]{ "", "hello", std::string(11, 'a'), std::string(12, 'b'), std::string(4096, 'c'), "A line\nwith UTF-8: \xE2\x98\x85" };
	for (const bool transfer : { false, true }) {
		consume = transfer;
		for (const auto& message : messages) {
			expected = message;
			RE::DebugMessageBox(message.c_str());
			Check(allocations.empty(), "Request leaked a string allocation");
		}
		expected.clear();
		RE::DebugMessageBox(nullptr);
		Check(allocations.empty(), "Null input leaked an allocation");
	}
	Check(creates == 14 && destroys == 14, "Each request must be created and destroyed exactly once");
	std::cout << "PASS: 14 requests, inline/heap strings, null input, native ownership, and missing manager\n";
}
