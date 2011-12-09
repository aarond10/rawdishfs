RawDish Filesystem
==================

RawDish is a peer-to-peer distributed filesystem with disk (and later network) encryption and snapshots.

The intention is to support [DropBox](http://www.dropbox.com)-like use cases, allowing you to share your storage with friends and family such that if any one (or potentially several) sites were lost, your data is still safe.

Unlike DropBox, our implementation requires no central server and you can form your own network amongst friends, free from the potentially prying eyes of DropBox employees and foreign governments alike.

Initially we won't be supporting concurrent mounting of the same filesystem at different sites but this is something we'd like to eventually do (akin to DropBox shared folders). 

If you find the concept behind this project interesting, feel free to drop me an email. Volunteers are more than welcome.

Disclaimer
----------

This software is still ALPHA. Use at your own risk. 

Dependencies
------------

I've tried to keep dependencies fairly limited unless they really add to the project. msgpack is used for RPC serialization, openssl for encryption, google logging for debugging and gtest for unit tests.

On an ubuntu/debian box, you can probably install everything you need with the following:

    $ sudo apt-get install libmsgpack-dev libssl-dev libgoogle-glog-dev libgtest-dev

Design
------

### Unresolved Design Issues ###

The coding scheme we'll be using is still under discussion but we'll be trying to keep it as simple as possible. We had originally planned to run with Reed-Solomon encoding on a per-block level but having thought about it a bit more have realised that RS will give us improved reliability *per block* but the reliability of large files will still be BlockReliability^NumberOfBlocks which is terrible in the large file case. The current plan is to use something akin to fountain codes with stronger guarantees on "orthogonality" than the general case. How exactly we do this is still being discussed. Needless-to-say, we're thinking long and hard about this for the data's sake. :)

### BlockStore layer ###

This is a full-mesh network of peers, each providing one or more BlockStore objects of a fixed (currently 16GB) size. Each BlockStore is given a random 64-bit BSID. Disks that are larger than 16GB are broken up into a set of these 16GB BlockStore's, each with a random BSID.

Each node in the full-mesh network keeps its own synchronised copy of BloomFilters for every BlockStore in the network. These contain the hashed names of each block a BlockStore contains. Each node also exposes on a machine-local interface an RPC service that allows the following operations:

  - GetFreeSpace
  - SetBlock
  - GetBlock
  - SetGCBloomFilter
  - GetMissingBlocks

#### GetFreeSpace ####

Returns the amount of free space in the whole network. This is the sum of the free space in all known BlockStores. There is no way (and no reason) to find out the amount of free space in an individual BlockStore.

#### SetBlock ####

The use of this RPC is fairly self explanatory but the working under-the-bonnet is a bit more involved.

When SetBlock is called, the local node hashes the block name in 2 different ways and in both cases finds the BlockStore with the closest BSID *below* the hash value. We will call these BS-A, BS-B. We attempt to store the block in the BlockStore with the most free space. If a BlockStore exceeds 95% capacity, we apply a techinique similar to [Cuckoo Hashing](http://en.wikipedia.org/wiki/Cuckoo_hashing) to evict blocks from the full BS and rebalance free space across the network.

Storing a block involves writing it to disk (currently a file on a filesystem), updating the BS's BloomFilter and transmitting it to all other mesh nodes.

#### GetBlock ####

We first perform the same block name hashing as in SetBlock and seach for the closes BlockStores with BSID's below the hash value. (Lets call them BS-X and BS-Y.) Note that we may have had new BlockStore's added to the network since our data was inserted such that BS-A != BS-X and/or BS-B != BS-Y. Since we can't assume we have the right locations, we have to start a search, begining with a local check of the BloomFilter's for BS-X and BS-Y, then BS-X - 1, BS-Y - 1, and so on until we either find what we want or run out of options.  When we find a BloomFilter match, we query the node directly to confirm we didn't just hit a false positive result from the filter.

#### SetGCBloomFilter ####

Sets a (large!) BloomFilter that contains all blocks from all filesystems in the network. We perform continuous incremental scans across BlockStores at each node and during this process (amongst other things) anything that this filter doesn't match is added to a Garbage Collection Candidate List. We don't actually delete these blocks until we reach 100% capacity, then we will start replacing them with new data. This gives us more of a chance to recover from catastrophies should they occur.

*Note*: We still don't know how or who will create this filter. Sending a bad GCBloomFilter could potentially be very dangerous so this feature may be disabled in early releases.

#### GetMissingBlocks ####

As part of the incremental scan across local BlockStores, each node tracks blocks coded in known formats. If a file is broken up into 100 parts (including coded redundant parts) and a given node has part n, it searches its BloomFilter for part (n + 1) % 100. If found in a BloomFilter, the block is assumed valid. The BS BloomFilter's kept by the network are occasionally re-keyed so its unlikely that a missing block will go unnoticed for long and the chance of a false positive in a well sized bloom-filter is small enough that we simply accept them (for now at least). Any parts that _cannot_ be found elsewhere in the network are added to an internal list. This RPC returns the list. 

The intended use-case is for the layer above, which is responsible for coding of data, to periodically call this RPC and for each entry returned, pull down enough other blocks to reconstruct the missing data and reinject it into the network.

### BlockStore Design Rationale ###

  1. Using fixed size BlockStore's of moderate size allows us to balance data across the network using random hash-based placement.
  2. Using cuckoo hash inspired evictions deals with imbalance and allows us to make better use of new, empty devices added to the mesh.
  3. Lookup involves local in-memory scanning of BloomFilters at known locations and occasional network queries. 
  4. Regardless of the number of BlockStore nodes we have, we have a fairly small search space to check (we have a starting point guaranteed to be close to our data for stable networks).
  5. The use of a large BloomFilter for GC just has elegant characteristics that suit our use-case. We don't want to keep state. We don't want to pass around lots of data. We can accept delayed deletions, etc..
  6. The periodic checking for missing blocks can be conveniently done along with GC and provides a clean, single point at which the layer above receives notification of block storage problems.
  
