#pragma once

#include <cstdint>

namespace Core {

class ISimSystem {
public:
    virtual ~ISimSystem() = default;
    virtual const char* getName() const = 0;
    virtual void onTick(uint64_t tickCount) = 0;
    uint64_t getLastTickCount() const;

protected:
    uint64_t lastTickCount = 0;
};

class DebugSystem final : public ISimSystem {
public:
    const char* getName() const override;
    void onTick(uint64_t tickCount) override;
    uint64_t getProcessedTickCount() const;

private:
    uint64_t processedTickCount = 0;
};

class SystemRegistry {
public:
    SystemRegistry();
    void initialize();
    void runTick(uint64_t tickCount);
    int32_t getSystemCount() const;
    const ISimSystem* getSystem(int32_t index) const;

private:
    DebugSystem debugSystem;
    ISimSystem* systems[8];
    int32_t systemCount;
};

} // namespace Core
