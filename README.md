RawDish Filesystem
==================

RawDish is a peer-to-peer distributed filesystem with full disk and network encryption, block-level de-duplication and snapshots.

The intention is to support [DropBox](http://www.dropbox.com)-like use cases, allowing you to share your storage with friends and family such that if any one (or potentially several) sites were lost, your data is still safe.

Initially we won't be supporting concurrent mounting of the same filesystem at different sites but this is something we'd like to eventually do (akin to DropBox shared folders).

Disclaimer
----------

This software is still ALPHA. Use at your own risk. 

Dependencies
------------

I've tried to keep dependencies fairly limited unless they really add to the project. msgpack is used for RPC serialization, google logging for debugging and gtest for unit tests.

On an ubuntu/debian box, you can probably get away with:

    $ sudo apt-get install libmsgpack-dev libgoogle-glog-dev libgtest-dev

