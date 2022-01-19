Title: Thread Safety

# Thread Safety

Aravis is not thread safe, which means one can not use the same object
simultaneously from different threads, without using mutexes. But it is
perfectly fine to use different aravis objects in different threads, with the
exception of the ArvInterface instances.

A possible trap is that glib signal callbacks are called from the thread that
emitted the corresponding signal. For example, the "new-buffer" callback is
emitted from the stream packet receiving thread, which is an internal thread
created by ArvGvStream. It is not safe to use ArvDevice and some ArvStream
functions from this callback without using mutexes. The exceptions are
arv_stream_push_buffer(), arv_stream_pop_buffer(), arv_stream_try_pop_buffer()
and arv_stream_timeout_pop_buffer().
