#include <quad_encoder/driver.hpp>

#include <math.h>

using namespace quad_encoder;

// CONSTRUCTORS
driver::driver(uint32_t cpr)
    : m_transition_matrix{{0, 1, -1, 2}, {1, 0, 2, -1}, {-1, 2, 0, 1}, {2, 1, -1, 0}},
      m_cpr(cpr),
      m_prior_state(0),
      m_current_position(0),
      m_pulses_missed(0)
{}

// STATE UPDATE
void driver::initialize_state(bool level_a, bool level_b)
{
    // Lock state thread protection.
    driver::m_mutex_state.lock();

    // Initialize the prior state with the current state.
    driver::m_prior_state = (static_cast<int8_t>(level_a) << 1) | static_cast<int8_t>(level_b);

    // Unlock state thread protection.
    driver::m_mutex_state.unlock();
}
void driver::tick_a(bool level)
{
    // Calculate the new state.
    int8_t new_state = (driver::m_prior_state & 1) | (static_cast<int8_t>(level) << 1);

    // Update the encoder's state with the new state.
    driver::update_state(new_state);
}
void driver::tick_b(bool level)
{
    // Calculate the new state.
    int8_t new_state = (driver::m_prior_state & 2) | static_cast<int8_t>(level);

    // Update the encoder's state with the new state.
    driver::update_state(new_state);
}
void driver::update_state(int8_t new_state)
{
    // Lock state thread protection.
    driver::m_mutex_state.lock();

    // Get the transition value.
    int8_t transition = driver::m_transition_matrix[driver::m_prior_state][new_state];

    // Check if there was a valid transition.
    if(transition == 2)
    {
        driver::m_pulses_missed += 1;
    }
    else
    {
        // Update the position.
        driver::m_current_position += transition;
    }

    // Update the prior state.
    driver::m_prior_state = new_state;

    // Unlock state thread protection.
    driver::m_mutex_state.unlock();
}

// ACCESS
void driver::set_home()
{
    // Lock state thread protection.
    driver::m_mutex_state.lock();

    // Set current position to zero.
    driver::m_current_position = 0;
    
    // Reset the missed pulses flag.
    driver::m_pulses_missed = 0;

    // Unlock state thread protection.
    driver::m_mutex_state.unlock();
}
double driver::get_position(bool reset)
{
    // Lock state thread protection.
    driver::m_mutex_state.lock();

    // Get current position in CPR.
    double position = static_cast<double>(driver::m_current_position);
    if(reset)
    {
        driver::m_current_position = 0;
        driver::m_pulses_missed = 0;
    }

    // Unlock state thread protection.
    driver::m_mutex_state.unlock();

    // Return position in radians.
    return position / static_cast<double>(driver::m_cpr) * 2.0 * M_PIf64;
}
uint64_t driver::pulses_missed()
{
    // Lock state thread protection.
    std::lock_guard<std::mutex> lock_guard(driver::m_mutex_state);
    
    return driver::m_pulses_missed;
}