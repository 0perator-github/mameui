// For licensing and usage information, read docs/winui_license.txt
// ============================================================================

// ============================================================================
// swconfig.h
// ============================================================================

#ifndef MAMEUI_WINAPP_SWCONFIG_H
#define MAMEUI_WINAPP_SWCONFIG_H

class software_config
{
public:

	software_config(int driver_index);

	~software_config() = default;

	int driver_index() const
	{
		return m_driver_index;
	}

	const game_driver *game_driver_ptr() const
	{
		return m_game_driver;
	}

	const machine_config *machine_config_ptr() const
	{
		return m_machine_config.get();
	}

private:

	int m_driver_index;

	const game_driver *m_game_driver;

	std::unique_ptr<machine_config> m_machine_config;
};

using software_config_ptr = std::unique_ptr<software_config>;

#endif // MAMEUI_WINAPP_SWCONFIG_H
