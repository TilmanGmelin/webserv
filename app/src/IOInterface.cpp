#include "../include/Webserv.hpp"

void webs::IOInterface::Dispatch()
{
	Listen();
	uint32_t segment_start = SocketCount();

	// recieve data on open connections
	RecieveData(segment_start);

	// send data on open connections
	segment_start += ReadConCount();
	SendData(segment_start);

	// read data from files
	segment_start += WriteConCount();
	ReadFile(segment_start);
	
	// write data to files
	segment_start += ReadFileCount();
	WriteFile(segment_start);

	// delete entries marked for delete
	// and add newly created entries
	FlushChanges();
}

inline void webs::IOInterface::Listen()
{
	uint32_t lim = SocketCount();
	for (int i = 0; i < lim; i++)	
	{
		if (__builtin_expect(open_fds_[i].revents & POLLIN, true))
		{
			uint32_t new_conection = accept(open_fds_[i].fd, nullptr, nullptr);

			if (new_conection < 0)
				continue ;

			// new socket to no blocking
            fcntl(new_conection, F_SETFL, O_NONBLOCK);

			// queue new connection to be added
			new_read_connections_.push_back({new_conection, ports_[i]});
		}
	}
}

inline void webs::IOInterface::RecieveData(const uint32_t& _start_index)
{
	uint32_t lim = ReadConCount();
	for (int i = 0; i < lim; i++)
	{
		// mark errournous and closed connections for deletion
		// this could be moved to a simd scan ... just because!
		if (__builtin_expect(open_fds_[i + _start_index].revents & POLLERR, false))
		{
			deleted_elements_++;
			open_fds_[i + _start_index].events = 0;
			continue ;
		}

		if (open_fds_[i + _start_index].revents & (POLLIN | POLLHUP))
		{
			char		 buffer[RECV_BUFFER_SIZE];
			ssize_t		 bytes_read;
			uint32_t	 fd = open_fds_[i + _start_index].fd;
			std::string* data = recieved_data_[i];

			while ((bytes_read = recv(fd, buffer, RECV_BUFFER_SIZE, 0)) > 0) // while new data is read
			{
				if (__builtin_expect(data->size() >= MAX_DATA_SIZE, false)) // if data exceeds max data size
				{
					open_fds_[i + _start_index].events = 0;
					break ;
				}
				data->append(buffer, bytes_read);
			}

			// handle closed connection
			if (bytes_read == 0)
			{
				deleted_elements_++;
				open_fds_[i + _start_index].events = 0;
				server_controller_->Dispatch(data, fd, connection_ports_[i]);
			}
		}
	}
}

inline void webs::IOInterface::SendData(const uint32_t& _start_index)
{
	uint32_t lim = WriteConCount();
	for (int i = 0; i < lim; i++)
	{
		// mark errournous and closed connections for deletion
		// this could be moved to a simd scan ... just because!
		if (__builtin_expect(open_fds_[i + _start_index].revents & (POLLERR | POLLHUP), false))
		{
			deleted_elements_++;
			open_fds_[i + _start_index].events = 0;
			continue ;
		}
		
		if (open_fds_[i + _start_index].revents & (POLLOUT)) // if fd is rdy for data to be writen to it
		{
			uint32_t data_size 		= responses_[i].data->size();
			uint32_t bytes_written 	= responses_[i].bytes_written;
			char*	 data 			= responses_[i].data->data();
			uint32_t fd				= open_fds_[i + _start_index].fd;
			uint32_t chunk_size;

			while (chunk_size = data_size - bytes_written) // while data to be written is remaining
			{
				if (__builtin_expect(chunk_size > RECV_BUFFER_SIZE, true)) // clamp ammount to be written
					chunk_size = RECV_BUFFER_SIZE;
				uint32_t bytes_send = send(fd, data + bytes_written, chunk_size, 0);
				if (bytes_send == -1) // check for send error (also blocking)
				{
					// cannot send further data
					break ;
				}
				bytes_written += bytes_send;
			}
			responses_[i].bytes_written = bytes_written;
			if (bytes_written == data_size) // if all data has been send
			{
				open_fds_[i + _start_index].events = 0; // mark connection for deletion
				deleted_elements_++;
			}
		}
	}
}

inline void webs::IOInterface::ReadFile(const uint32_t& _start_index)
{
	uint32_t lim = ReadFileCount();
	for (int i = 0; i < lim; i++)
	{
		// mark errournous and closed connections for deletion
		// this could be moved to a simd scan ... just because!
		if (__builtin_expect(open_fds_[i + _start_index].revents & POLLERR, false))
		{
			open_fds_[i + _start_index].events = 0;
			continue ;
		}

		if (open_fds_[i + _start_index].revents & (POLLIN | POLLHUP))
		{
			char		 buffer[RECV_BUFFER_SIZE];
			ssize_t		 bytes_read;
			uint32_t	 fd = open_fds_[i + _start_index].fd;
			std::string* data = read_data_[i];

			while ((bytes_read = read(fd, buffer, RECV_BUFFER_SIZE)) > 0) // while new data is read
			{
				data->append(buffer, bytes_read);
				if (__builtin_expect(data->size() >= MAX_DATA_SIZE, false)) // if data exceeds max data size
				{
					open_fds_[i + _start_index].events = 0;
					open_fds_[i + _start_index].revents = INTERNAL_SERVER_ERROR;
					deleted_elements_++;
					break ;
				}
			}

			// handle closed connection
			if (__builtin_expect(bytes_read == 0, false))
			{
				open_fds_[i + _start_index].events = 0;
				deleted_elements_++;
				server_controller_->Dispatch(data, fd, connection_ports_[i]);
			}
		}
	}
}

inline void webs::IOInterface::WriteFile(const uint32_t& _start_index)
{
	uint32_t lim = WriteFileCount();
	for (int i = 0; i < lim; i++)
	{
		// mark errournous and closed connections for deletion
		// this could be moved to a simd scan ... just because!
		if (__builtin_expect(open_fds_[i + _start_index].revents & POLLERR, false))
		{
			open_fds_[i + _start_index].events = 0;
			continue ;
		}
		
		if (open_fds_[i + _start_index].revents & (POLLOUT)) // if fd is rdy for data to be writen to it
		{
			uint32_t data_size 		= write_data_[i].data->size();
			uint32_t bytes_written 	= write_data_[i].bytes_written;
			char*	 data 			= write_data_[i].data->data();
			uint32_t fd				= open_fds_[i + _start_index].fd;
			uint32_t chunk_size;

			while (chunk_size = data_size - bytes_written) // while data to be written is remaining
			{
				if (__builtin_expect(chunk_size > RECV_BUFFER_SIZE, true)) // clamp ammount to be written
					chunk_size = RECV_BUFFER_SIZE;
				uint32_t bytes_send = write(fd, data + bytes_written, chunk_size);
				if (bytes_send == -1) // check for send error (also blocking)
				{
					// cannot send further data
					break ;
				}
				bytes_written += bytes_send;
			}
			write_data_[i].bytes_written = bytes_written;
			if (bytes_written == data_size) // if all data has been send
			{
				deleted_elements_++;
				open_fds_[i + _start_index].events = 0; // mark connection for deletion
			}
		}
	}
}


void webs::IOInterface::FlushChanges()
{
}