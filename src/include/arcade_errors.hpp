#pragma once

#include <string>
namespace Arcade {

	enum class ErrorType {
		NULL_ERROR = 0,
		CRIT_ERROR = 1,
		FILE_ERROR = 2,
		ARG_ERROR = 3,
		// File errors
			FILE_NOT_FOUND = 200,
			FILE_NO_PERMISSION = 201,
			FILE_EOF = 202,
			FILE_BAD_MAGIC = 203,
			FILE_READONLY = 204,
			FILE_WRITEONLY = 205,
			FILE_WRONG_METHOD = 206,
		// Argument errors
			ARG_LENGTH = 300
	};

	template <ErrorType T>
	class Error {
		ErrorType subtype{};
		std::string msg;
	public:

		Error( std::string message ) : msg(message) {}
		Error( std::string message, ErrorType sub ) : msg(message), subtype(sub) {}

		const std::string_view what() {return msg;}
		const ErrorType type() {return subtype;}
	};

}