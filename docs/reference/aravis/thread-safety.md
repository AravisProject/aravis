Title: Thread Safety

# Thread Safety

Aravis is not thread safe, which means one can not use the same object
simultaneously from different threads, without using mutexes. But it is
perfectly fine to use different aravis objects in different threads, with the
exception of the [class@Aravis.Interface] instances.

A possible trap is that glib signal callbacks are called from the thread that
emitted the corresponding signal. For example, the
[signal@Aravis.Stream::new-buffer] callback is emitted from the stream packet
receiving thread, which is an internal thread created by
[class@Aravis.GvStream]. It is not safe to use [class@Aravis.Device] and some
[class@Aravis.Stream] functions from this callback without using mutexes. The
exceptions are [method@Aravis.Stream.push_buffer],
[method@Aravis.Stream.pop_buffer], [method@Aravis.Stream.try_pop_buffer] and
[method@Aravis.Stream.timeout_pop_buffer].
