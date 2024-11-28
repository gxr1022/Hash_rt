perf record -e cache-misses,cache-references /mnt/nvme0/home/gxr/Myhash_boc/Hash_rt/build/hash_rt

perf stat -e cache-misses,cache-references /mnt/nvme0/home/gxr/Myhash_boc/Hash_rt/build/hash_rt

# perf stat -e L1-dcache-loads,L1-dcache-load-misses,LLC-loads,LLC-load-misses /mnt/nvme0/home/gxr/Myhash_boc/Hash_rt/build/hash_rt