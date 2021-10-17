
# Newsletter

Second-year bachelor's degree second assignament for Communication Protocols course.

It contains:
- A multiplexed server able to receive UDP packages and to broadcast them towards all the subscribed clients. 
- TCP clients able to (un)subscribe from/to certain topics. While online they would receive any broadcasted messages related to their topics.
While offline the server should keep track of what messages were not sent and then send them in bulk the next time the user opens up its client.
