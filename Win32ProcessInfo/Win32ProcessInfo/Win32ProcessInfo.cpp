#include <iostream>
#include <array>
#include <cassert>
#include <vector>

#include <Windows.h>
#include <Psapi.h>

static constexpr DWORD PROCESS_ACCESS_FLAGS
{
	PROCESS_QUERY_INFORMATION | PROCESS_VM_READ
};

static constexpr std::size_t MAX_PROCESSES { 1 << 14 };
static constexpr std::size_t MAX_MODULES { 1 << 14 };

static auto PrintProcessInfo(const DWORD id) -> void
{
	const HANDLE process { OpenProcess(PROCESS_ACCESS_FLAGS, FALSE, id) };
	if (!process)
	{
		[[unlikely]]
			return;
	}

	const auto queryName
	{
		[=]() -> std::wstring
		{
			std::array<TCHAR, MAX_PATH> nameBuf { L"Unknown" };
			HMODULE                     module;
			DWORD                       needed;
			if (EnumProcessModules(process, &module, sizeof module, &needed))
			{
				GetModuleBaseName(process, module, std::data(nameBuf), static_cast<DWORD>(std::size(nameBuf)));
			}
			return { std::data(nameBuf) };
		}
	};

	const auto querySubmodules
	{
		[=]() -> std::vector<std::wstring>
		{
			std::vector<HMODULE> modules { };
			modules.resize(MAX_MODULES);

			DWORD count;
			if (EnumProcessModules(process, std::data(modules), sizeof(HMODULE) * MAX_MODULES, &count))
			{
				count /= sizeof(DWORD);
				modules.resize(count);
				modules.shrink_to_fit();
				std::vector<std::wstring> result { };
				result.reserve(count);
				for (std::size_t i { 0 }; i < count; ++i)
				{
					std::array<TCHAR, MAX_PATH> nameBuf { L"Unknown" };
					if (GetModuleFileNameEx(process, modules[i], std::data(nameBuf), static_cast<DWORD>(std::size(nameBuf))))
					{
						result.emplace_back(std::wstring { std::data(nameBuf) });
					}
				}
				result.shrink_to_fit();
				return result;
			}
			return { };
		}
	};

	std::wcout << "Process: " << queryName() << ", ID: " << std::hex << id << '\n';
	for (std::size_t i { 0 }; auto&& module : querySubmodules())
	{
		std::wcout << "\tModule " << ++i << ": " << module << '\n';
	}
	std::wcout << std::endl;

	assert(CloseHandle(process));
}

auto main() -> int
try
{
	std::ostream::sync_with_stdio(false);
	std::wostream::sync_with_stdio(false);

	std::vector<DWORD> processes { };
	processes.resize(MAX_PROCESSES);

	DWORD count;
	if (!EnumProcesses(std::data(processes), sizeof(DWORD) * MAX_PROCESSES, &count) || !count)
	{
		return -1;
	}

	count /= sizeof(DWORD);
	std::wcout << "Found " << count << " processes!\n";

	processes.resize(count);
	processes.shrink_to_fit();

	for (const DWORD id : processes)
	{
		PrintProcessInfo(id);
	}

	return 0;
}
catch (const std::exception& ex)
{
	std::cout << ex.what() << '\n';
	return -1;
}
catch (...)
{
	return -1;
}
