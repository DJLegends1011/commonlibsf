#include "RE/C/ControlMap.h"

#include <cstring>

namespace RE
{
	namespace
	{
		struct alignas(8) MappingRowString
		{
			~MappingRowString()
			{
				static REL::Relocation<void (*)(MappingRowString*)> destroy{ ID::ControlMap::MappingRowStringDtor };
				destroy(this);
			}

			[[nodiscard]] const char* c_str() const
			{
				if (capacity <= sizeof(storage)) {
					return storage;
				}
				const char* data;
				std::memcpy(&data, storage, sizeof(data));
				return data;
			}

			char          storage[0xC]{};
			std::uint16_t capacity{ 1 };
			std::uint16_t unk0E{};
		};
		static_assert(sizeof(MappingRowString) == 0x10);
		static_assert(offsetof(MappingRowString, capacity) == 0xC);
	}

	std::string ControlMap::FormatMappingRow(
		const char* a_event, const char* a_keyboard, const char* a_mouse, const char* a_gamepad,
		bool a_keyboardVisible, bool a_mouseVisible, bool a_gamepadVisible,
		std::uint32_t a_controlMask, std::uint32_t a_groupMask, bool a_required)
	{
		using func_t = MappingRowString* (*)(MappingRowString*, const char*, const char*, const char*, const char*,
			bool, bool, bool, std::uint32_t, std::uint32_t, bool);
		static REL::Relocation<func_t> func{ ID::ControlMap::FormatMappingRow };
		MappingRowString               text;
		func(&text, a_event, a_keyboard, a_mouse, a_gamepad,
			a_keyboardVisible, a_mouseVisible, a_gamepadVisible, a_controlMask, a_groupMask, a_required);
		return text.c_str();
	}

	std::string ControlMap::FormatMappingRow(
		const char* a_event, std::uint32_t a_keyboard, std::uint32_t a_mouse, std::uint32_t a_gamepad,
		bool a_keyboardVisible, bool a_mouseVisible, bool a_gamepadVisible,
		std::uint32_t a_controlMask, std::uint32_t a_groupMask, bool a_required)
	{
		using func_t = MappingRowString* (*)(MappingRowString*, const char*, std::uint32_t, std::uint32_t, std::uint32_t,
			bool, bool, bool, std::uint32_t, std::uint32_t, bool);
		static REL::Relocation<func_t> func{ ID::ControlMap::FormatMappingRowWithKeyCodes };
		MappingRowString               text;
		func(&text, a_event, a_keyboard, a_mouse, a_gamepad,
			a_keyboardVisible, a_mouseVisible, a_gamepadVisible, a_controlMask, a_groupMask, a_required);
		return text.c_str();
	}

	void ControlMap::LoadMappings()
	{
		using func_t = decltype(&ControlMap::LoadMappings);
		static REL::Relocation<func_t> func{ ID::ControlMap::LoadMappings };
		func(this);
	}

	void ControlMap::ParseMappings(const char* a_text)
	{
		using func_t = decltype(&ControlMap::ParseMappings);
		static REL::Relocation<func_t> func{ ID::ControlMap::ParseMappings };
		func(this, a_text);
	}

	void ControlMap::ResolveLinkedMappings()
	{
		using func_t = decltype(&ControlMap::ResolveLinkedMappings);
		static REL::Relocation<func_t> func{ ID::ControlMap::ResolveLinkedMappings };
		func(this);
	}
}
