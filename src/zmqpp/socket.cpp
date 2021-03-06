/*
 *  Created on: 9 Aug 2011
 *      Author: Ben Gray (@benjamg)
 */

#include <array>
#include <cassert>
#include <cstring>
#include <functional>

#include "context.hpp"
#include "exception.hpp"
#include "message.hpp"
#include "socket.hpp"

namespace zmqpp
{

const int socket::NORMAL;
const int socket::DONT_WAIT;
const int socket::SEND_MORE;
#ifdef ZMQ_EXPERIMENTAL_LABELS
const int socket::SEND_LABEL;
#endif

const int max_socket_option_buffer_size = 256;
const int max_stream_buffer_size = 4096;

socket::socket(const context& context, socket_type const& type)
	: _socket(nullptr)
	, _type(type)
	, _recv_buffer()
{
	_socket = zmq_socket(context, static_cast<int>(type));
	if(nullptr == _socket)
	{
		throw zmq_internal_exception();
	}

	zmq_msg_init(&_recv_buffer);
}

socket::~socket()
{
	zmq_msg_close(&_recv_buffer);

	if (nullptr != _socket)
	{

#ifndef NDEBUG // unused assert variable in release
		int result = zmq_close(_socket);
		assert(0 == result);
#else
		zmq_close(_socket);
#endif // NDEBUG

		_socket = nullptr;
	}
}

void socket::bind(endpoint_t const& endpoint)
{
	int result = zmq_bind(_socket, endpoint.c_str());

	if (0 != result)
	{
		throw zmq_internal_exception();
	}
}

void socket::connect(endpoint_t const& endpoint)
{
	int result = zmq_connect(_socket, endpoint.c_str());

	if (0 != result)
	{
		throw zmq_internal_exception();
	}
}

void socket::close()
{
	int result = zmq_close(_socket);

	if (0 != result)
	{
		throw zmq_internal_exception();
	}

	_socket = nullptr;
}

bool socket::send(message& other, bool const& dont_block /* = false */)
{
	// we wish to take ownership passed in object valid but leave so swap
	message local;
	std::swap(local, other);

	size_t parts = local.parts();
	if (parts == 0)
	{
		throw std::invalid_argument("sending requires messages have at least one part");
	}

	for(size_t i = 0; i < parts; ++i)
	{
		int flag = socket::NORMAL;
		if(dont_block) { flag |= socket::DONT_WAIT; }
		if(i < (parts - 1)) { flag |= socket::SEND_MORE; }

#if (ZMQ_VERSION_MAJOR == 3) and (ZMQ_VERSION_MINOR == 0)
		int result = zmq_sendmsg(_socket, &local.raw_msg(i), flag);
#else
		int result = zmq_msg_send(&local.raw_msg(i), _socket, flag);
#endif

		if (result < 0)
		{
			// the zmq framework should not block if the first part is accepted
			// so we should only ever get this error on the first part
			if((0 == i) && (EAGAIN == zmq_errno()))
			{
				return false;
			}

			// sanity checking
			assert(EAGAIN != zmq_errno());

			throw zmq_internal_exception();
		}

		local.sent(i);
	}

	return true;
}

bool socket::receive(message& message, bool const& dont_block /* = false */)
{
	if (message.parts() > 0)
	{
		throw exception("receiving can only be done to empty messages");
	}

	int flags = (dont_block) ? socket::DONT_WAIT : socket::NORMAL;
	bool more = true;

	while(more)
	{
		int result = zmq_recvmsg(_socket, &_recv_buffer, flags);

		if(result < 0)
		{
			if ((0 == message.parts()) && (EAGAIN == zmq_errno()))
			{
				return false;
			}

			assert(EAGAIN == zmq_errno());

			throw zmq_internal_exception();
		}

		zmq_msg_t& dest = message.raw_new_msg();
		zmq_msg_move(&dest, &_recv_buffer);

		get(socket_option::receive_more, more);
	}

	return true;
}


bool socket::send(std::string const& string, int const& flags /* = NORMAL */)
{
	int result = zmq_send(_socket, string.data(), string.size(), flags);

	if(result >= 0)
	{
		return true;
	}

	if (EAGAIN == zmq_errno())
	{
		return false;
	}

	throw zmq_internal_exception();
}


bool socket::receive(std::string& string, int const& flags /* = NORMAL */)
{
	int result = zmq_recvmsg(_socket, &_recv_buffer, flags);

	if(result >= 0)
	{
		assert(static_cast<size_t>(result) == zmq_msg_size(&_recv_buffer));

		string.reserve(result);
		string.assign(static_cast<char*>(zmq_msg_data(&_recv_buffer)), result);
		return true;
	}

	if (EAGAIN == zmq_errno())
	{
		return false;
	}

	throw zmq_internal_exception();
}


bool socket::send_raw(char const* buffer, int const& length, int const& flags /* = NORMAL */)
{
	int result = zmq_send(_socket, buffer, length, flags);

	if(result >= 0)
	{
		return true;
	}

	if (EAGAIN == zmq_errno())
	{
		return false;
	}

	throw zmq_internal_exception();
}

bool socket::receive_raw(char* buffer, int& length, int const& flags /* = NORMAL */)
{
	int result = zmq_recvmsg(_socket, &_recv_buffer, flags);

	if(result >= 0)
	{
		assert(static_cast<size_t>(result) == zmq_msg_size(&_recv_buffer));

		memcpy(buffer, zmq_msg_data(&_recv_buffer), result);
		length = result;

		return true;
	}

	if (EAGAIN == zmq_errno())
	{
		return false;
	}

	throw zmq_internal_exception();
}


// Helper
void socket::subscribe(std::string const& topic)
{
	set(socket_option::subscribe, topic);
}

void socket::unsubscribe(std::string const& topic)
{
	set(socket_option::unsubscribe, topic);
}

bool socket::has_more_parts() const
{
	return get<bool>(socket_option::receive_more);
}


// Set socket options for different types of option
void socket::set(socket_option const& option, int const& value)
{
	switch(option)
	{
	case socket_option::affinity:
		if (value < 0)
			throw exception("attempting to set an unsigned 64 bit integer option with a negative integer");
		set(option, static_cast<uint64_t>(value));
		break;
	case socket_option::send_high_water_mark:
	case socket_option::receive_high_water_mark:
	case socket_option::rate:
	case socket_option::send_buffer_size:
	case socket_option::receive_buffer_size:
	case socket_option::linger:
	case socket_option::backlog:
	case socket_option::recovery_interval:
	case socket_option::reconnect_interval:
	case socket_option::reconnect_interval_max:
	case socket_option::max_messsage_size:
	case socket_option::multicast_hops:
	case socket_option::receive_timeout:
	case socket_option::send_timeout:
#if (ZMQ_VERSION_MAJOR > 3) or ((ZMQ_VERSION_MAJOR == 3) and (ZMQ_VERSION_MINOR >= 1))
	case socket_option::ipv4_only:
#endif
		zmq_setsockopt(_socket, static_cast<int>(option), &value, sizeof(value));
		break;
	default:
		throw exception("attempting to set a non signed integer option with a signed i<< ohint->owner->_stringsnteger value");
	}
}

#if (ZMQ_VERSION_MAJOR > 3) or ((ZMQ_VERSION_MAJOR == 3) and (ZMQ_VERSION_MINOR >= 1))
void socket::set(socket_option const& option, bool const& value)
{
	switch(option)
	{
	case socket_option::ipv4_only:
		zmq_setsockopt(_socket, static_cast<int>(option), &value, sizeof(value));
		break;
	default:
		throw exception("attempting to set a non boolean option with a boolean value");
	}
}
#endif

void socket::set(socket_option const& option, uint64_t const& value)
{
	switch(option)
	{
	case socket_option::affinity:
		zmq_setsockopt(_socket, static_cast<int>(option), &value, sizeof(value));
		break;
	default:
		throw exception("attempting to set a non unsigned 64 bit integer option with a unsigned 64 bit integer value");
	}
}

void socket::set(socket_option const& option, std::string const& value)
{
	switch(option)
	{
	case socket_option::identity:
	case socket_option::subscribe:
	case socket_option::unsubscribe:
		zmq_setsockopt(_socket, static_cast<int>(option), value.c_str(), value.length());
		break;
	default:
		throw exception("attempting to set a non string option with a string value");
	}
}


// Get socket options, multiple versions for easy of use
void socket::get(socket_option const& option, int& value) const
{
	size_t value_size = sizeof(int);

	switch(option)
	{
	case socket_option::type:
	case socket_option::receive_more:
	case socket_option::send_high_water_mark:
	case socket_option::receive_high_water_mark:
	case socket_option::rate:
	case socket_option::recovery_interval:
	case socket_option::send_buffer_size:
	case socket_option::receive_buffer_size:
	case socket_option::linger:
	case socket_option::backlog:
	case socket_option::reconnect_interval:
	case socket_option::reconnect_interval_max:
	case socket_option::max_messsage_size:
	case socket_option::multicast_hops:
	case socket_option::receive_timeout:
	case socket_option::send_timeout:
	case socket_option::file_descriptor:
	case socket_option::events:
#if (ZMQ_VERSION_MAJOR > 3) or ((ZMQ_VERSION_MAJOR == 3) and (ZMQ_VERSION_MINOR >= 1))
	case socket_option::ipv4_only:
#endif
#ifdef ZMQ_EXPERIMENTAL_LABELS
	case socket_option::receive_label:
#endif
		zmq_getsockopt(_socket, static_cast<int>(option), &value, &value_size);

		// sanity check
		assert(value_size <= sizeof(int));
		break;
	default:
		throw exception("attempting to get a non integer option with an integer value");
	}
}

void socket::get(socket_option const& option, bool& value) const
{
	int int_value = 0;
	size_t value_size = sizeof(int);

	switch(option)
	{
	case socket_option::receive_more:
#if (ZMQ_VERSION_MAJOR > 3) or ((ZMQ_VERSION_MAJOR == 3) and (ZMQ_VERSION_MINOR >= 1))
	case socket_option::ipv4_only:
#endif
#ifdef ZMQ_EXPERIMENTAL_LABELS
	case socket_option::receive_label:
#endif
		zmq_getsockopt(_socket, static_cast<int>(option), &int_value, &value_size);

		value = (int_value == 1) ? true : false;
		break;
	default:
		throw exception("attempting to get a non boolean option with a boolean value");
	}
}

void socket::get(socket_option const& option, uint64_t& value) const
{
	size_t value_size = sizeof(uint64_t);

	switch(option)
	{
	case socket_option::affinity:
		zmq_getsockopt(_socket, static_cast<int>(option), &value, &value_size);
		break;
	default:
		throw exception("attempting to get a non unsigned 64 bit integer option with an unsigned 64 bit integer value");
	}
}

void socket::get(socket_option const& option, std::string& value) const
{
	static std::array<char, max_socket_option_buffer_size> buffer;
	size_t size = max_socket_option_buffer_size;

	switch(option)
	{
	case socket_option::identity:
		zmq_getsockopt(_socket, static_cast<int>(option), buffer.data(), &size);

		value.assign(buffer.data(), size);
		break;
	default:
		throw exception("attempting to get a non string option with a string value");
	}
}

socket::socket(socket&& source) noexcept
	: _socket(source._socket)
	, _type(source._type)
	, _recv_buffer()
{
	// we steal the zmq_msg_t from the valid socket, we only init our own because it's cheap
	// and zmq_msg_move does a valid check
	zmq_msg_init(&_recv_buffer);
	zmq_msg_move(&_recv_buffer, &source._recv_buffer);

	// Clean up source a little
	source._socket = nullptr;
}

socket& socket::operator=(socket&& source) noexcept
{
	_socket = source._socket;
	source._socket = nullptr;

	_type = source._type; // just clone?

	return *this;
}


socket::operator bool() const
{
	return nullptr != _socket;
}


socket::operator void*() const
{
	return _socket;
}

void socket::track_message(message const& /* message */, uint32_t const& parts, bool& should_delete)
{
	if (parts == 0)
	{
		should_delete = true;
	}
}

}
