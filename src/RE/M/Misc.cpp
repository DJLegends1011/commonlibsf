#include "RE/M/Misc.h"

namespace RE
{
	class IMessageBoxCallback;
	class MessageMenuManager;

	namespace
	{
		// The native request uses BSStringT<char, 1, DynamicMemoryManagementPol>.
		// Its inline storage differs from the BSStringT currently exposed by this
		// library. Keep the ABI private and let the engine assign and destroy it.
		struct MessageBoxString
		{
			std::byte     storage[0xC]{};  // 00: inline characters or owned pointer
			std::uint16_t capacity{ 1 };   // 0C: includes the terminator
			std::uint16_t length{};        // 0E
		};
		static_assert(sizeof(MessageBoxString) == 0x10);
		static_assert(offsetof(MessageBoxString, capacity) == 0xC);
		static_assert(offsetof(MessageBoxString, length) == 0xE);

		struct MessageBoxParams
		{
			MessageBoxParams() = default;
			MessageBoxParams(const MessageBoxParams&) = delete;
			MessageBoxParams& operator=(const MessageBoxParams&) = delete;

			~MessageBoxParams()
			{
				using func_t = void (*)(MessageBoxParams*);
				static REL::Relocation<func_t> func{ ID::MessageMenuManager::DestroyMessageBoxParams };
				func(this);
			}

			IMessageBoxCallback* callback{};           // 00
			MessageBoxString     header;               // 08
			MessageBoxString     body;                 // 18
			MessageBoxString     unk28;                // 28
			std::uint32_t        warningContext{ 5 };  // 38: script
		};
		static_assert(sizeof(MessageBoxParams) == 0x40);
		static_assert(offsetof(MessageBoxParams, callback) == 0x00);
		static_assert(offsetof(MessageBoxParams, header) == 0x08);
		static_assert(offsetof(MessageBoxParams, body) == 0x18);
		static_assert(offsetof(MessageBoxParams, unk28) == 0x28);
		static_assert(offsetof(MessageBoxParams, warningContext) == 0x38);
	}

	void DebugMessageBox(const char* a_message)
	{
		static REL::Relocation<MessageMenuManager**> singleton{ ID::MessageMenuManager::Singleton };
		auto                                         manager = *singleton;
		if (!manager) {
			return;
		}

		MessageBoxParams params;
		using assign_t = bool (*)(MessageBoxString*, const char*, std::size_t);
		static REL::Relocation<assign_t> assign{ ID::BSStringT::Assign };
		assign(&params.header, "DEBUG", 0);
		assign(&params.body, a_message, 0);

		// This overload supplies the localized OK button and moves the owned
		// strings/callback into its request. The destructor handles either state.
		using create_t = void (*)(MessageMenuManager*, MessageBoxParams*, bool);
		static REL::Relocation<create_t> create{ ID::MessageMenuManager::CreateMessageBox };
		create(manager, &params, false);
	}
}
