_pybgpstream
============

This document describes the API of the _pybgpstream module, a low-level
(almost) direct interface to the C `libbgpstream` library. For most uses, the
`pybgpstream` module should be used instead.

.. py:module:: _pybgpstream

BGPStream
---------

.. py:class:: BGPStream

   The BGP Stream class provides a single stream of BGP Records.

   .. py:method:: add_filter(type, value)

      Add a filter to an unstarted BGP Stream instance. Only those records that
      match the filter(s) will be included in the stream.

      If multiple filters of the **same** type are added, a record is considered
      a match if it matches **any** of the filters. E.g. if
      `add_filter('project', 'routeviews')` and `add_filter('project', 'ris')`
      are used, then records that are from either the `Route Views`, **or** the
      `RIS` project will be included.

      If multiple filters of **different** types are added, a record is
      considered a match if it matches **all** of the filters. E.g. if
      `add_filter('project', 'routeviews')` and `add_filter('record-type',
      'updates')` are used, then records that are both from the `Route Views`
      project, **and** are `updates` will be included.

      :param str type: The type of the filter, can be one of `project`,
		       `collector`, `record-type`
      :param str value: The value of the filter
      :raises TypeError: if the type or value are not basestrings
      :raises ValueError: if the type is not valid


   .. py:method:: add_interval_filter(start, stop)

      Add an interval filter to an unstarted BGP Stream instance. Only those
      records that fall within the given interval will be included in the
      stream.

      If multiple interval filters are added, then a record is included if it is
      inside **any** of the intervals.

      :param int start: The start time of the interval (inclusive)
      :param int stop: The end time of the interval (exclusive)
      :raises TypeError: if the start or end times are not ints


   .. py:method:: get_data_interfaces()

      Gets a list of information about the available data interfaces.
      Each item in the list will have three fields: `id`, `name`, and
      `description`. The value of the `name` field can be used in subsequent
      calls to :py:meth:`set_data_interface`.

   .. py:method:: set_data_interface(interface-name)

      Sets the data interface to stream BGP Records from.

      :param str interface: The data interface to use, must be one of the `name`
                            fields in the result of
                            :py:meth:`get_data_interfaces`.
      :raises TypeError: if the interface is not a basestring
      :raises ValueError: if the given interface is not valid


   .. py:method:: set_data_interface_option(type, value)

      Sets a data interface option.

      :param str type: The option to set, can be one of `mysql-db`,
		       `mysql-user`, `mysql-host`, `csvfile-file`.
      :param str value: The value to set the option to.
      :raises TypeError: if the type or value are not basestrings
      :raises ValueError: if the given type is not valid


   .. py:method:: set_blocking()

      Enables blocking mode. When this option is used, the stream will block
      waiting for new data to arrive if the end of the interval has not been
      reached. In this way a stream can be used to monitor realtime data (i.e. a
      call to :py:meth:`get_next_record` will block until new data is
      available.)


   .. py:method:: start()

      Starts the stream. This method must be called **after** all configuration
      options have been set (e.g. filters, options, etc.), and **before** the
      first call to :py:meth:`get_next_record`.


   .. py:method:: get_next_record(record)

      Retrieves the next record from the stream, and stores the result into the
      given record object. Passing a record instance helps reduce the allocation
      overhead of this method. If the records are processing independently of
      each other, then the same record instance may be used for subsequent calls
      to this method. If the blocking mode is enabled, then this method may
      block if the stream reaches the end of the data available in the archive,
      and the end of the interval(s) has not been reached.

      :param BGPRecord record: A record instance into which the next record from
			       the stream is stored.
      :return: True if there are more records in the stream, False if the end of
	       the stream has been reached.
      :rtype: bool
      :raises RuntimeError: if the provided record instance is invalid, if the
			    stream has not been started, or if the stream
			    encounters an error retrieving the next record

BGPRecord
---------

.. py:class:: BGPRecord

   The BGP Record class represents a single record obtained from a BGP
   Stream.

   All attributes are read-only.


   .. py:attribute:: project

      The name of the project that created the record. *(basestring, readonly)*


   .. py:attribute:: collector

      The name of the collector that created the record. *(basestring, readonly)*


   .. py:attribute:: type

      The type of the record, can be one of 'update', 'rib', or 'unknown'.
      *(basestring, readonly)*


   .. py:attribute:: dump_time

      The time associated with the dump that contained the record (e.g. the
      beginning of the MRT file that the record was found in.) *(int, readonly)*


   .. py:attribute:: time

      The time that the record represents (i.e. the time the record was
      generated by the collector). *(int, readonly)*


   .. py:attribute:: status

      The status of the record, can be one of 'valid', 'filtered-source',
      'empty-source', 'corrupted-source', 'unknown'. *(basestring, readonly)*


   .. py:attribute:: dump_position

      The position that this record was found in the dump, can be one of
      'start', 'middle', 'end', 'unknown'. *(basestring, readonly)*


   .. py:method:: get_next_elem()

      Get the next :py:class:`BGPElem` from this record. Will return
      :py:class:`None` when all the elems have been read.

      :return: a :py:class:`BGPElem` object, or `None` if there are no more
               elems to read.
      :rtype: :py:class:`BGPElem`
      :raises RuntimeError: if a BGPElem object could not be created



BGPElem
---------

.. py:class:: BGPElem

   The BGP Elem class represents a single element obtained from a BGP Record
   instance using the :py:meth:`BGPRecord.get_next_elem` method.

   All attributes are read-only.


   .. py:attribute:: type

      The type of the element, can be one of 'rib', 'announcement',
      'withdrawal', 'peerstate', 'unknown'. *(basestring, readonly)*


   .. py:attribute:: time

      The time that the element represents. *(int, readonly)*


   .. py:attribute:: peer_address

      The IP address of the peer that this element was received
      from. *(basestring, readonly)*


   .. py:attribute:: peer_asn

      The ASN of the peer that this element was received from. *(int, readonly)*


   .. py:attribute:: fields

      A dictionary of fields that differ depending on the :py:attr:`type` of the
      element. *(dict, readonly)*

      Fields for each type are:
         - *rib*, *announcement*:
            - 'next-hop': The next-hop IP address (basestring)
            - 'as-path': The AS path (basestring)
            - 'prefix': The prefix (basestring)
	 - *withdrawal*
            - 'prefix': The prefix (basestring)
	 - *peerstate*
            - 'old-state': The old state of the peer, can be one of 'idle',
	      'connect', 'active', 'open-sent', 'open-confirm', 'established'.
	      (basestring)
            - 'new-state': The new state of the peer, shares the same possible
	      values as old-state. (basestring)
