#pragma once
#include <stdexcept>

struct InitializationException : public std::runtime_error
{
	InitializationException(std::string const& err) : runtime_error(err) {}
};

struct InvalidStateException : public std::runtime_error
{
	InvalidStateException(std::string const& err) : runtime_error(err) {}
};