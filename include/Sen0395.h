#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include <string>

typedef void (*DetectionCallback)( bool );

class Sen0395
{
public:
    static Sen0395 *GetInstance();
    static void Start();
    void OnDetectionChange(DetectionCallback callback) { m_callback = callback; m_callback(m_isDetected); }
    bool IsDetected() { return m_isDetected; }
protected:
    Sen0395();
    bool SendCommand(const std::string& cmd);
    void Initialize();
    static Sen0395* s_instance;
    static void EventHandler(void*);
    bool m_isDetected = false;
    QueueHandle_t m_uartQueue;
    TaskHandle_t m_taskHandle;
    DetectionCallback m_callback = nullptr;
};