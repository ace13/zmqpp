/**
 * \file
 *
 * \date   9 Aug 2011
 * \author Ben Gray (\@benjamg)
 */

#ifndef ZMQPP_POLLER_HPP_
#define ZMQPP_POLLER_HPP_

#include <unordered_map>
#include <vector>

#include "compatibility.hpp"

namespace zmqpp
{

class socket;
typedef socket socket_t;

/*!
 * Polling wrapper.
 *
 * Allows access to polling for any number of sockets or file descriptors.
 */
class poller
{
public:
	static const long WAIT_FOREVER; /*!< Block forever flag, default setting. */

	static const short POLL_NONE;   /*!< No polling flags set. */
	static const short POLL_IN;     /*!< Monitor inbound flag. */
	static const short POLL_OUT;    /*!< Monitor output flag. */
	static const short POLL_ERROR;  /*!< Monitor error flag.\n Only for file descriptors. */

	/*!
	 * Construct an empty polling model.
	 */
	poller();

	/*!
	 * Cleanup poller.
	 *
	 * Any sockets will need to be closed separately.
	 */
	~poller();

	/*!
	 * Add a socket to the polling model and set which events to monitor.
	 *
	 * \param socket the socket to monitor.
	 * \param event the event flags to monitor on the socket.
	 */
	void add(socket_t& socket, short const& event = POLL_IN);

	/*!
	 * Add a file descriptor to the polling model and set which events to monitor.
	 *
	 * \param descriptor the file descriptor to monitor.
	 * \param event the event flags to monitor.
	 */
	void add(int const& descriptor, short const& event = POLL_IN | POLL_ERROR );

	/*!
	 * Update the monitored event flags for a given socket.
	 *
	 * \param socket the socket to update event flags.
	 * \param event the event flags to monitor on the socket.
	 */
	void check_for(socket_t const& socket, short const& event);

	/*!
	 * Update the monitored event flags for a given file descriptor.
	 *
	 * \param descriptor the file descriptor to update event flags.
	 * \param event the event flags to monitor on the socket.
	 */
	void check_for(int const& descriptor, short const& event);

	/*!
	 * Poll for monitored events.
	 *
	 * By default this method will block forever or until at least one of the monitored
	 * sockets or file descriptors has events.
	 *
	 * If a timeout is set and was reached then this function returns false.
	 *
	 * \param timeout milliseconds to timeout.
	 * \return true if there is an event..
	 */
	bool poll(long timeout = WAIT_FOREVER);

	/*!
	 * Get the event flags triggered for a socket.
	 *
	 * \param socket the socket to get triggered event flags for.
	 * \return the event flags.
	 */
	short events(socket_t const& socket) const;

	/*!
	 * Get the event flags triggered for a file descriptor.
	 *
	 * \param descriptor the file descriptor to get triggered event flags for.
	 * \return the event flags.
	 */
	short events(int const& descriptor) const;

	/*!
	 * Check either a file descriptor or socket for input events.
	 *
	 * Templated helper method that calls through to event and checks for a given flag
	 *
	 * \param watchable either a file descriptor or socket known to the poller.
	 * \return true if there is input.
	 */
	template<typename Watched>
	bool has_input(Watched const& watchable) const { return events(watchable) & POLL_IN; }

	/*!
	 * Check either a file descriptor or socket for output events.
	 *
	 * Templated helper method that calls through to event and checks for a given flag
	 *
	 * \param watchable either a file descriptor or socket known to the poller.
	 * \return true if there is output.
	 */
	template<typename Watched>
	bool has_output(Watched const& watchable) const { return events(watchable) & POLL_OUT; }

	/*!
	 * Check a file descriptor.
	 *
	 * Templated helper method that calls through to event and checks for a given flag
	 *
	 * Technically this template works for sockets as well but the error flag is never set for
	 * sockets so I have no idea why someone would call it.
	 *
	 * \param watchable a file descriptor know to the poller.
	 * \return true if there is an error.
	 */
	template<typename Watched>
	bool has_error(Watched const& watchable) const { return events(watchable) & POLL_ERROR; }

private:
	std::vector<zmq_pollitem_t> _items;
	std::unordered_map<void *, size_t> _index;
	std::unordered_map<int, size_t> _fdindex;
};

}

#endif /* ZMQPP_POLLER_HPP_ */
