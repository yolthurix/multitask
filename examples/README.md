# Examples

## IO multiplexer
Uses poll to multiplex sockets. When an event occurs the `poll_events` function queues the callback which handles the event.

Only handles connection establishments for now.
