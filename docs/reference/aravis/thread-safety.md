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

## Genicam DOM tree

The Genicam data are a DOM tree, which is write locked once the device is
created, using [method@Aravis.DomDocument.set_write_lock]. While the tree is
write locked:

- any modification of the tree is refused: node insertion or removal,
  attribute or text change, or document loading
  ([method@Aravis.DomNode.append_child], [method@Aravis.DomNode.insert_before],
  [method@Aravis.DomNode.remove_child], [method@Aravis.DomNode.replace_child],
  [method@Aravis.DomElement.set_attribute],
  [method@Aravis.DomCharacterData.set_data],
  [method@Aravis.DomNode.set_node_value],
  [func@Aravis.dom_document_append_from_memory],
  [method@Aravis.Gc.set_default_node_data]...).

- the read accessors of the tree are thread safe, and the strings and lists
  they return are valid until the write lock is released: node names,
  attributes, text, child node lists, descriptions, tooltips, display names,
  enumeration entries ([method@Aravis.GcEnumeration.get_entries]), category
  features ([method@Aravis.GcCategory.get_features]) and selected features
  ([method@Aravis.GcSelector.get_selected_features]). The data built lazily
  by these accessors are built once, at the first call.

The write lock can be released, for example in order to modify the tree of a
device, after which the tree goes back to a regular, not thread safe, DOM
tree. Changing the write lock is not thread safe, and must be done while the
tree is not used from another thread.

The evaluation functions of the [class@Aravis.GcPropertyNode] (value getters
and setters) and the [class@Aravis.Evaluator] are not covered by the write
lock, as they may access the device, and are not thread safe.
