/**
 * \file
 *
 * \date   9 Aug 2011
 * \author Ben Gray (\@benjamg)
 */

#ifndef ZMQPP_SOCKET_HPP_
#define ZMQPP_SOCKET_HPP_

#include <string>
#include <list>

#include <zmq.h>

#include "compatibility.hpp"

#include "socket_types.hpp"
#include "socket_options.hpp"

namespace zmqpp
{

class context;
class message;

typedef std::string endpoint_t;
typedef context     context_t;
typedef message     message_t;

/*!
 * The socket class represents the zmq sockets.
 *
 * A socket can be bound and/or connected to as many endpoints as required
 * with the sole exception of a ::pair socket.
 *
 * The routing is handled by zmq based on the type set.
 *
 * The bound side of an inproc connection must occur first and inproc can only
 * connect to other inproc sockets of the same context.
 *
 * This class is c++0x move supporting and cannot be copied.
 */
class socket
{
public:
	static const int NORMAL     = 0;            /*!< /brief default send type, no flags set */
	static const int DONT_WAIT  = ZMQ_DONTWAIT; /*!< /brief don't block if sending is not currently possible  */
	static const int SEND_MORE  = ZMQ_SNDMORE;  /*!< /brief more parts will follow this one */
#ifdef ZMQ_EXPERIMENTAL_LABELS
	static const int SEND_LABEL = ZMQ_SNDLABEL; /*!< /brief this message part is an internal zmq label */
#endif

	/*!
	 * Create a socket for a given type.
	 *
	 * \param context the zmq context under which the socket will live
	 * \param type a valid ::socket_type for the socket
	 */
	socket(context_t const& context, socket_type const& type);

	/*!
	 * This will close any socket still open before returning
	 */
	~socket();

	/*!
	 * Get the type of the socket, this works on zmqpp types and not the zmq internal types.
	 * Use the socket::get method if you wish to intergoate the zmq internal ones.
	 *
	 * \return the type of the socket
	 */
	socket_type type() const { return _type; }

	/*!
	 * Asynchronously binds to an endpoint.
	 *
	 * \param endpoint the zmq endpoint to bind to
	 */
	void bind(endpoint_t const& endpoint);

	/*!
	 * Asynchronously connects to an endpoint.
	 * If the endpoint is not inproc then zmq will happily keep trying
	 * to connect until there is something there.
	 *
	 * Inproc sockets must have a valid target already bound before connection
	 * will work.
	 *
	 * \param endpoint the zmq endpoint to connect to
	 */
	void connect(endpoint_t const& endpoint);

	/*!
	 * Asynchronously connects to multiple endpoints.
	 * If the endpoint is not inproc then zmq will happily keep trying
	 * to connect until there is something there.
	 *
	 * Inproc sockets must have a valid target already bound before connection
	 * will work.
	 *
	 * This is a helper function that wraps the single item connect in a loop
	 *
	 * \param connections_begin the starting iterator for zmq endpoints.
	 * \param connections_end the final iterator for zmq endpoints.
	 */
	template<typename InputIterator>
	void connect(InputIterator const& connections_begin, InputIterator const& connections_end)
	{
		for(InputIterator it = connections_begin; it != connections_end; ++it)
		{
			connect(*it);
		}
	}

	/*!
	 * Closes the internal zmq socket and marks this instance
	 * as invalid.
	 */
	void close();

	/*!
	 * Sends the message over the connection, this may be a multipart message.
	 *
	 * If dont_block is true and we are unable to add a new message then this
	 * function will return false.
	 *
	 * \param message message to send
	 * \param dont_block boolean to dictate if we wait while sending.
	 * \return true if message sent, false if it would have blocked
	 */
	bool send(message_t& message, bool const& dont_block = false);

	/*!
	 * Gets a message from the connection, this may be a multipart message.
	 *
	 * If dont_block is true and we are unable to get a message then this
	 * function will return false.
	 *
	 * \param message reference to fill with received data
	 * \param dont_block boolean to dictate if we wait for data.
	 * \return true if message sent, false if it would have blocked
	 */
	bool receive(message_t& message, bool const& dont_block = false);

	/*!
	 * Sends the byte data held by the string as the next message part.
	 *
	 * If the socket::DONT_WAIT flag and we are unable to add a new message to
	 * socket then this function will return false.
	 *
	 * \param string message part to send
	 * \param flags message send flags
	 * \return true if message part sent, false if it would have blocked
	 */
	bool send(std::string const& string, int const& flags = NORMAL);

	/*!
	 * If there is a message ready then get the next part as a string
	 *
	 * If the socket::DONT_WAIT flag and there is no message ready to receive
	 * then this function will return false.
	 *
	 * \param string message part to receive into
	 * \param flags message receive flags
	 * \return true if message part received, false if it would have blocked
	 */
	bool receive(std::string& string, int const& flags = NORMAL);

	/*!
	 * Sends the byte data pointed to by buffer as the next part of the message.
	 *
	 * If the socket::DONT_WAIT flag and we are unable to add a new message to
	 * socket then this function will return false.
	 *
	 * \param buffer byte buffer pointer to start writing from
	 * \param length max length of the buffer
	 * \param flags message send flags
	 * \return true if message part sent, false if it would have blocked
	 */
	bool send_raw(char const* buffer, int const& length, int const& flags = NORMAL);

	/*!
	 * \warning If the buffer is not large enough for the message part then the
	 *       data will be truncated. The rest of the part is lost forever.
	 *
	 * If there is a message ready then get the next part of it as a raw
	 * byte buffer.
	 *
	 * If the socket::DONT_WAIT flag and there is no message ready to receive
	 * then this function will return false.
	 *
	 * \param buffer byte buffer pointer to start writing to
	 * \param length max length of the buffer
	 * \param flags message receive flags
	 * \return true if message part received, false if it would have blocked
	 */
	bool receive_raw(char* buffer, int& length, int const& flags = NORMAL);

	/*!
	 *
	 * Subscribe to a topic
	 *
	 * Helper function that is equivalent of calling
	 * \code
	 * set(zmqpp::socket_option::subscribe, topic);
	 * \endcode
	 *
	 * This method is only useful for subscribe type sockets.
	 *
	 * \param topic the topic to subscribe to.
	 */
	void subscribe(std::string const& topic);

	/*!
	 * Subscribe to a topic
	 *
	 * Helper function that is equivalent of a loop calling
	 * \code
	 * set(zmqpp::socket_option::subscribe, topic);
	 * \endcode
	 *
	 * This method is only useful for subscribe type sockets.
	 *
	 * Generally this will be used with stl collections using begin() and
	 * end() functions to get the iterators.
	 * For this reason the end loop runs until the end iterator, not inclusive
	 * of it.
	 *
	 * \param topics_begin the starting iterator for topics.
	 * \param topics_end the final iterator for topics.
	 */
	template<typename InputIterator>
	void subscribe(InputIterator const& topics_begin, InputIterator const& topics_end)
	{
		for(InputIterator it = topics_begin; it != topics_end; ++it)
		{
			subscribe(*it);
		}
	}

	/*!
	 * Unsubscribe from a topic
	 *
	 * Helper function that is equivalent of calling
	 * \code
	 * set(zmqpp::socket_option::unsubscribe, topic);
	 * \endcode
	 *
	 * This method is only useful for subscribe type sockets.
	 *
	 * \param topic the topic to unsubscribe from.
	 */
	void unsubscribe(std::string const& topic);

	/*!
	 * Unsubscribe from a topic
	 *
	 * Helper function that is equivalent of a loop calling
	 * \code
	 * set(zmqpp::socket_option::unsubscribe, topic);
	 * \endcode
	 *
	 * This method is only useful for subscribe type sockets.
	 *
	 * Generally this will be used with stl collections using begin() and
	 * end() functions to get the iterators.
	 * For this reason the end loop runs until the end iterator, not inclusive
	 * of it.
	 *
	 * \param topics_begin the starting iterator for topics.
	 * \param topics_end the final iterator for topics.
	 */
	template<typename InputIterator>
	void unsubscribe(InputIterator const& topics_begin, InputIterator const& topics_end)
	{
		for(InputIterator it = topics_begin; it != topics_end; ++it)
		{
			unsubscribe(*it);
		}
	}

	/*!
	 * If the last receive part call to the socket resulted
	 * in a label or a non-terminating part of a multipart
	 * message this will return true.
	 *
	 * \return true if there are more parts
	 */
	bool has_more_parts() const;

	/*!
	 * Set the value of an option in the underlaying zmq socket.
	 *
	 * \param option a valid ::socket_option
	 * \param value to set the option to
	 */
	void set(socket_option const& option, int const& value);

#if (ZMQ_VERSION_MAJOR > 3) or ((ZMQ_VERSION_MAJOR == 3) and (ZMQ_VERSION_MINOR >= 1))
	/*!
	 * Set the value of an option in the underlaying zmq socket.
	 *
	 * \since 2.0.0 (built against 0mq version 3.1.x or later)
	 *
	 * \param option a valid ::socket_option
	 * \param value to set the option to
	 */
	void set(socket_option const& option, bool const& value);
#endif

	/*!
	 * Set the value of an option in the underlaying zmq socket.
	 *
	 * \param option a valid ::socket_option
	 * \param value to set the option to
	 */
	void set(socket_option const& option, uint64_t const& value);

	/*!
	 * Set the value of an option in the underlaying zmq socket.
	 *
	 * \param option a valid ::socket_option
	 * \param value to set the option to
	 */
	void set(socket_option const& option, std::string const& value);

	/*!
	 * Get a socket option from the underlaying zmq socket.
	 *
	 * \param option a valid ::socket_option
	 * \param value referenced int to return value in
	 */
	void get(socket_option const& option, int& value) const;

	/*!
	 * Get a socket option from the underlaying zmq socket.
	 *
	 * \param option a valid ::socket_option
	 * \param value referenced bool to return value in
	 */
	void get(socket_option const& option, bool& value) const;

	/*!
	 * Get a socket option from the underlaying zmq socket.
	 *
	 * \param option a valid ::socket_option
	 * \param value referenced uint64_t to return value in
	 */
	void get(socket_option const& option, uint64_t& value) const;

	/*!
	 * Get a socket option from the underlaying zmq socket.
	 *
	 * \param option a valid ::socket_option
	 * \param value referenced std::string to return value in
	 */
	void get(socket_option const& option, std::string& value) const;

	/*!
	 * For those that don't want to get into a referenced value this templated method
	 * will return the value instead.
	 *
	 * \param option a valid ::socket_option
	 * \return socket option value
	 */
	template<typename Type>
	Type get(socket_option const& option) const
	{
		Type value;
		get(option, value);
		return value;
	}

	/*!
	 * Move constructor
	 *
	 * Moves the internals of source to this object, there is no guaranty
	 * that source will be left in a valid state.
	 *
	 * This constructor is noexcept and so will not throw exceptions
	 *
	 * \param source target socket to steal internals from
	 */
	socket(socket&& source) noexcept;

	/*!
	 * Move operator
	 *
	 * Moves the internals of source to this object, there is no guaranty
	 * that source will be left in a valid state.
	 *
	 * This function is noexcept and so will not throw exceptions
	 *
	 * \param source target socket to steal internals from
	 * \return socket reference to this
	 */
	socket& operator=(socket&& source) noexcept;

	/*!
	 * Check the socket is still valid
	 *
	 * This tests the internal state of the socket.
	 * If creation failed for some reason or if the move functions were used
	 * to move the socket internals to another instance this will return false.
	 *
	 * \return true if the socket is valid
	 */
	operator bool() const;

	/*!
	 * Access to the raw 0mq context
	 *
	 * \return void pointer to the underlying 0mq socket
	 */
	operator void*() const;

private:
	void* _socket;
	socket_type _type;
	zmq_msg_t _recv_buffer;

	// No copy
	socket(socket const&) noexcept;
	socket& operator=(socket const&) noexcept;

	void track_message(message_t const&, uint32_t const&, bool&);
};

}

#endif /* ZMQPP_SOCKET_HPP_ */
