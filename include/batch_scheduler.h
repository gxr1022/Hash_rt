class BatchScheduler {
private:
    static constexpr size_t BATCH_SIZE = 1000;  // 批处理大小
    std::atomic<size_t> pending_ops{0};
    std::mutex mutex;
    std::condition_variable cv;

public:
    void schedule_batch(ExtendibleHash* hashTable, 
                       size_t start_idx, 
                       size_t end_idx,
                       const std::vector<std::pair<std::string, std::string>>& ops)
    {
        pending_ops += (end_idx - start_idx);

        for (size_t i = start_idx; i < end_idx; i++) {
            const auto& [key, value] = ops[i];
            hashTable->insert(key.c_str(), value.c_str(), 0);
            
            if (--pending_ops == 0) {
                std::unique_lock<std::mutex> lock(mutex);
                cv.notify_one();
            }
        }
    }

    void wait_batch_complete() {
        std::unique_lock<std::mutex> lock(mutex);
        cv.wait(lock, [this]() { return pending_ops == 0; });
    }
};