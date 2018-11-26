/**
 * @file ITimerFactory.h
 *
 * @brief Abstract interface to which must conforms implementations of factory classes that generate ITimer objects
 *
 * Used as a dependency inversion paradigm
 */
#pragma once

#include "ITimer.h"
#include <memory>	// For std::unique_ptr

/**
 * @brief Abstract factory to create ITimer instances
 */
class ITimerFactory {
public:
	/**
	 * @brief Destructor
	 */
	virtual ~ITimerFactory() { };

	/**
	 * @brief Create a new ITimer instance
	 *
	 * @return The new instance allocated
	 */
	virtual std::unique_ptr<ITimer> create() const = 0;
};
