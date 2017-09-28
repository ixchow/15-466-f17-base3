#include "Mode.hpp"

std::shared_ptr< Mode > Mode::current;

void Mode::set_current(std::shared_ptr< Mode > const &new_current) {
	current = new_current;
	//TODO: e.g., resize events on new current mode.
}
