#pragma once

#ifndef __CEFHEADLESSRENDERER__
#define __CEFHEADLESSRENDERER__

#include <iostream>
#include <thread>
#include <atomic>
#include <thread>
#include <mutex>
#include <semaphore.h>
#include <condition_variable>
#include <include/cef_app.h>
#include <include/cef_client.h>
#include <include/wrapper/cef_message_router.h>
#include <include/wrapper/cef_resource_manager.h>
#include <stdlib.h>

namespace CEF
{
    using namespace std;

    // Set Argument overrides
    void SetArgument(CefRefPtr<CefListValue> args, int idx, const char *user_arg)
    {
        args->SetString(idx, (const string)(user_arg));
    }

    void SetArgument(CefRefPtr<CefListValue> args, int idx, const string user_arg)
    {
        args->SetString(idx, user_arg);
    }

    void SetArgument(CefRefPtr<CefListValue> args, int idx, int user_arg)
    {
        args->SetInt(idx, user_arg);
    }

    void SetArgument(CefRefPtr<CefListValue> args, int idx, bool user_arg)
    {
        args->SetBool(idx, user_arg);
    }

    void SetArgument(CefRefPtr<CefListValue> args, int idx, float user_arg)
    {
        args->SetDouble(idx, user_arg);
    }

    void SetArgument(CefRefPtr<CefListValue> args, int idx, double user_arg)
    {
        args->SetDouble(idx, user_arg);
    }

    // Convert CefList to V8List based on values type
    void CefListToV8List(CefListValue *source, CefV8ValueList *target, uint offset = 0)
    {
        int arg_length = static_cast<int>(source->GetSize());
        if (arg_length == 0)
            return;

        for (int i = offset; i < arg_length; ++i)
        {
            CefRefPtr<CefV8Value> new_value;

            CefValueType type = source->GetType(i);
            switch (type)
            {
            case VTYPE_BOOL:
                new_value = CefV8Value::CreateBool(source->GetBool(i));
                break;
            case VTYPE_DOUBLE:
                new_value = CefV8Value::CreateDouble(source->GetDouble(i));
                break;
            case VTYPE_INT:
                new_value = CefV8Value::CreateInt(source->GetInt(i));
                break;
            case VTYPE_STRING:
                new_value = CefV8Value::CreateString(source->GetString(i));
                break;
            case VTYPE_NULL:
                new_value = CefV8Value::CreateNull();
                break;
            default:
                break;
            }

            if (new_value.get())
            {
                target->push_back(new_value);
            }
            else
            {
                target->push_back(CefV8Value::CreateNull());
            }
        }
    }

    class MessageHandler : public CefMessageRouterBrowserSide::Handler
    {
    private:
        function<void()> _on_module_loaded;

    public:
        MessageHandler() = default;

        bool OnQuery(CefRefPtr<CefBrowser> browser,
                     CefRefPtr<CefFrame> frame,
                     int64 query_id,
                     const CefString &request,
                     bool persistent,
                     CefRefPtr<Callback> callback) override
        {
            if (request == "module_loaded")
            {
                if (_on_module_loaded != nullptr)
                {
                    puts("WebView Module Loaded");
                    _on_module_loaded();
                }
                return true;
            }
            return false;
        }

        void OnModuleLoaded(function<void()> callback)
        {
            _on_module_loaded = callback;
        }
    };

    void SetupResourceManagerOnIOThread(CefRefPtr<CefResourceManager> resourceManager)
    {
        if (!CefCurrentlyOn(TID_IO))
        {
            CefPostTask(TID_IO, base::Bind(SetupResourceManagerOnIOThread, resourceManager));
            return;
        }

    }

    ////////////////////////////////////////
    // ---- Renderer Process ----
    class RendererApp : public CefApp, public CefRenderProcessHandler
    {
    public:
        RendererApp() = default;

        CefRefPtr<CefRenderProcessHandler> GetRenderProcessHandler() override
        {
            return this;
        }

        void OnWebKitInitialized() override
        {
            // Render runs in a different process, thus IPC is needed to execute JS code
            puts("Chromium Embedded Framework Initialized");
            CefMessageRouterConfig config;
            m_messageRouter = CefMessageRouterRendererSide::Create(config);
        }

        virtual void OnContextCreated(CefRefPtr<CefBrowser> browser,
                                      CefRefPtr<CefFrame> frame, CefRefPtr<CefV8Context> context) override
        {
            // Inject Document loaded event
            frame->ExecuteJavaScript("window.addEventListener('DOMContentLoaded', (event) => {setTimeout(() => {console.log('WebView Document Ready'); window.cefQuery({request: 'module_loaded' });}, 10)});", "", 0);
            m_messageRouter->OnContextCreated(browser, frame, context);
        }

        virtual void OnContextReleased(CefRefPtr<CefBrowser> browser,
                                       CefRefPtr<CefFrame> frame, CefRefPtr<CefV8Context> context) override
        {
            m_messageRouter->OnContextReleased(browser, frame, context);
        }

        virtual bool OnProcessMessageReceived(CefRefPtr<CefBrowser> browser,
                                              CefRefPtr<CefFrame> frame,
                                              CefProcessId source_process,
                                              CefRefPtr<CefProcessMessage> message) override
        {
            auto name = message->GetName();
            auto args = message->GetArgumentList();
            if (!name.compare("fcn"))
            {
                auto fcn_name = args->GetString(0);
                CefV8ValueList fcn_args;
                CEF::CefListToV8List(args, &fcn_args, 1);

                auto v8Context = browser->GetMainFrame()->GetV8Context();
                if (v8Context->Enter())
                {
                    auto window = v8Context->GetGlobal();
                    auto fcn = window->GetValue(fcn_name);
                    fcn->ExecuteFunction(NULL, fcn_args);
                    v8Context->Exit();
                }
            }
            return m_messageRouter->OnProcessMessageReceived(browser, frame, source_process, message);
        }

    private:
        CefRefPtr<CefMessageRouterRendererSide> m_messageRouter;

        IMPLEMENT_REFCOUNTING(RendererApp);
        DISALLOW_COPY_AND_ASSIGN(RendererApp);
    };

    ////////////////////////////////////////
    // ---- Browser Process ----
    class HeadlessClient : public CefApp, public CefClient, public CefLifeSpanHandler, public CefRequestHandler, public CefResourceRequestHandler, public CefRenderHandler, public CefDisplayHandler
    {
    private:
        CefRefPtr<CefBrowser> _browser;
        CefRefPtr<CefCommandLine> _commandLine;
        CefRefPtr<CefApp> _app;
        CefSettings _settings;
        CefWindowInfo _windowInfo;
        CefBrowserSettings _browserSettings;
        thread *_cef_thread = nullptr;
        CefMainArgs *_args;

        std::mutex mutex_on_paint;
        std::condition_variable cv_on_paint;

        function<void(int width, int heigth, const void *image_data)> _on_paint_callback = nullptr;

        bool _low_priority = false;
        bool _resize_req = false;
        bool _resize_done = false;
        int _width = 400;
        int _height = 400;
        int _last_width;
        int _last_heigth;
        string _url;
        int _framerate = 30;
        bool _software_rendering = false;
        int _mouse_x;
        int _mouse_y;
        bool _logging;
        string _resources_path;
        mutex *_mutex_loop = nullptr;

        int ExecuteProcess(int argc, char *argv[])
        {
            _app = nullptr;
            _args = new CefMainArgs(argc, argv);
            _commandLine = CefCommandLine::CreateCommandLine();
            _commandLine->InitFromArgv(argc, argv);

            std::string appType = _commandLine->GetSwitchValue("type");
            if (appType == "renderer" || appType == "zygote")
            {
                _app = new RendererApp;
            }

            return CefExecuteProcess(*_args, _app, NULL);
        }

    public:
        CefRefPtr<CefLifeSpanHandler> GetLifeSpanHandler() override { return this; }
        CefRefPtr<CefRequestHandler> GetRequestHandler() override { return this; }
        CefRefPtr<CefRenderHandler> GetRenderHandler() override { return this; }
        CefRefPtr<CefDisplayHandler> GetDisplayHandler() override { return this; }

        bool running = false;

        HeadlessClient()
            : m_resourceManager(new CefResourceManager)
        {
            SetupResourceManagerOnIOThread(m_resourceManager);
        }

        int init(int argc, char *argv[], string initial_url = "",
                 int framerate = 30, bool software_rendering = false, bool enable_logging = false, string resources_dir = "",
                 bool low_priority = false)
        {
            // Execute the sub-process logic, if any. This will either return immediately for the browser
            // process or block until the sub-process should exit.
            if (ExecuteProcess(argc, argv) >= 0)
                exit(0);

            // Enable xhost
            // system("xhost + &> /dev/null");

            _url = initial_url;
            _framerate = framerate;
            _logging = enable_logging;
            _resources_path = resources_dir;
            _software_rendering = software_rendering;
            _low_priority = low_priority;

            sem_t sem_init_thread;
            sem_init(&sem_init_thread, 0, 0);

            _cef_thread = new thread([&]() {
                if (_low_priority)
                {
                    // Set schedule priority to IDLE
                    struct sched_param sp;
                    sp.sched_priority = sched_get_priority_min(SCHED_IDLE);
                    pthread_t this_thread = pthread_self();
                    sched_setscheduler(0, SCHED_IDLE, &sp);
                }

                _settings.windowless_rendering_enabled = 1;
                _settings.no_sandbox = true;

                if (_logging)
                    _settings.log_severity = LOGSEVERITY_INFO;
                else
                    _settings.log_severity = LOGSEVERITY_ERROR;

                char buf[128];
                char *cwd = getcwd(buf, sizeof(buf));
                std::ifstream _local_resource(cwd + "/bin/cef.pak"s);
                if (_local_resource.good())
                {
                    CefString(&_settings.resources_dir_path) = cwd + "/bin"s;
                    CefString(&_settings.locales_dir_path) = cwd + "/bin/locales"s;
                }
                else if (_resources_path.size() > 0)
                {
                    std::ifstream _ext_resource(_resources_path + "/cef.pak");
                    if (_ext_resource.good())
                    {
                        CefString(&_settings.resources_dir_path) = _resources_path;
                        CefString(&_settings.locales_dir_path) = _resources_path + "/locales";
                    }
                }

                CefInitialize(*_args, _settings, this, NULL);

                _windowInfo.SetAsWindowless(NULL);

                _browser = CefBrowserHost::CreateBrowserSync(_windowInfo, this, _url, _browserSettings, nullptr, nullptr);
                _browser->GetHost()->SetWindowlessFrameRate(_framerate);
                _browser->GetHost()->SetAudioMuted(true);
                running = true;
                sem_post(&sem_init_thread);
                while (running)
                {
                    if (_mutex_loop != nullptr)
                        _mutex_loop->lock();
                    CefDoMessageLoopWork();
                    if (_resize_req && _resize_done)
                    {
                        // Resize before loop starts if requested
                        _browser->GetHost()->WasResized();
                        _resize_req = false;
                        _resize_done = false;
                    }

                    if (_mutex_loop != nullptr)
                        _mutex_loop->unlock();

                    std::this_thread::sleep_for(std::chrono::milliseconds(10));
                }
            });
            pthread_setname_np(_cef_thread->native_handle(), "cef_thread");
            _cef_thread->detach();

            sem_wait(&sem_init_thread);
            sem_destroy(&sem_init_thread);

            return 0;
        }

        void OnBeforeCommandLineProcessing(
            const CefString &process_type,
            CefRefPtr<CefCommandLine> command_line) override
        {
            if (process_type.empty())
            {
                // Fix GPU options to improve performance
                if (_software_rendering)
                    command_line->AppendSwitch("disable-gpu");                                // Disable GPU
                command_line->AppendSwitch("disable-gpu-vsync");                              // Disable GPU vsync
                command_line->AppendSwitch("disable-gpu-early-init");                         // Disable proactive early init of GPU process.
                command_line->AppendSwitch("disable-gpu-shader-disk-cache");                  // Disables the GPU shader on disk cache.
                command_line->AppendSwitch("disable-gpu-compositing");                        // Prevent the compositor from using its GPU implementation.
                command_line->AppendSwitch("disable-gpu-memory-buffer-compositor-resources"); // Do not force that all compositor resources be backed by GPU memory buffers.
            }
        }

        string RelativeFileURL(const string file_path)
        {
            char buf[128];
            char *cwd = getcwd(buf, sizeof(buf));
            string url = "file://" + string(cwd) + "/" + file_path;
            return url;
        }

        void SetLoopMutex(mutex *loop_mutex)
        {
            _mutex_loop = loop_mutex;
        }

        void LoadURL(const char *page_url)
        {
            if (_browser)
            {
                _url = page_url;
                _browser->GetMainFrame()->LoadURL(page_url);
            }
        }

        void SetMouseMove(int x, int y, bool mouse_leave = false)
        {
            if (_browser)
            {
                CefMouseEvent event;
                event.x = x;
                event.y = y;
                _mouse_x = x;
                _mouse_y = y;

                _browser->GetHost()->SendMouseMoveEvent(event, mouse_leave);
            }
        }

        void SetMouseClick(int button_type, int mouse_clicked, int x = -1, int y = -1)
        {
            if (_browser)
            {
                CefMouseEvent event;
                if (x >= 0 && y >= 0)
                {
                    event.x = x;
                    event.y = y;
                }
                else
                {
                    event.x = _mouse_x;
                    event.y = _mouse_y;
                }
                CefBrowserHost::MouseButtonType btnType = (button_type == 0 ? MBT_LEFT : MBT_RIGHT);
                _browser->GetHost()->SendMouseClickEvent(event, btnType, !mouse_clicked, 1);
            }
        }

        void SetZoom(double zoom_value)
        {
            if (_browser)
            {
                _browser->GetHost()->SetZoomLevel(zoom_value);
            }
        }

        void SetScroll(int deltaX, int deltaY, int x = -1, int y = -1)
        {
            if (_browser)
            {
                CefMouseEvent event;
                if (x >= 0 && y >= 0)
                {
                    event.x = x;
                    event.y = y;
                }
                else
                {
                    event.x = _mouse_x;
                    event.y = _mouse_y;
                }
                _browser->GetHost()->SendMouseWheelEvent(event, deltaX, deltaY);
            }
        }

        void SetKeyPress()
        {
            // TODO
        }

        void SetFrameRate(int framerate)
        {
            if (_browser)
            {
                _browser->GetHost()->SetWindowlessFrameRate(framerate);
            }
        }

        void RefreshWindow()
        {
            if (_browser && _width && _height)
                _resize_req = true;
        }

        void SetViewSize(int width, int heigth)
        {
            if (_browser && (_width != width || _height != heigth))
            {
                _width = width;
                _height = heigth;
                _resize_req = true;
            }
        }

        void OnModuleLoaded(function<void()> callback)
        {
            if (m_messageHandler != nullptr)
            {
                ((MessageHandler *)m_messageHandler.get())->OnModuleLoaded(callback);
            }
        }

        // Register function to draw texture;
        void OnTextureUpdate(function<void(int width, int heigth, const void *image_data)> fcn)
        {
            if (fcn)
                _on_paint_callback = fcn;
        }

        bool OnProcessMessageReceived(CefRefPtr<CefBrowser> browser,
                                      CefRefPtr<CefFrame> frame,
                                      CefProcessId source_process,
                                      CefRefPtr<CefProcessMessage> message) override
        {
            return m_messageRouter->OnProcessMessageReceived(browser, frame, source_process, message);
        }

        /////////////////////////////////////
        // lifespan handler
        void OnAfterCreated(CefRefPtr<CefBrowser> browser) override
        {
            CefMessageRouterConfig mrconfig;
            m_messageRouter = CefMessageRouterBrowserSide::Create(mrconfig);
            m_messageHandler.reset(new MessageHandler);
            m_messageRouter->AddHandler(m_messageHandler.get(), false);
        }

        void OnBeforeClose(CefRefPtr<CefBrowser> browser) override
        {
            m_messageRouter->RemoveHandler(m_messageHandler.get());
            m_messageHandler.reset();
            m_messageRouter = nullptr;
        }

        /////////////////////////////////////
        // request handler

        bool OnBeforeBrowse(CefRefPtr<CefBrowser> browser,
                            CefRefPtr<CefFrame> frame,
                            CefRefPtr<CefRequest> request,
                            bool user_gesture,
                            bool is_redirect) override
        {
            m_messageRouter->OnBeforeBrowse(browser, frame);
            return false;
        }

        void OnRenderProcessTerminated(CefRefPtr<CefBrowser> browser, TerminationStatus status) override
        {
            m_messageRouter->OnRenderProcessTerminated(browser);
        }

        cef_return_value_t OnBeforeResourceLoad(
            CefRefPtr<CefBrowser> browser,
            CefRefPtr<CefFrame> frame,
            CefRefPtr<CefRequest> request,
            CefRefPtr<CefRequestCallback> callback) override
        {
            return m_resourceManager->OnBeforeResourceLoad(browser, frame, request, callback);
        }

        CefRefPtr<CefResourceHandler> GetResourceHandler(
            CefRefPtr<CefBrowser> browser,
            CefRefPtr<CefFrame> frame,
            CefRefPtr<CefRequest> request) override
        {
            return m_resourceManager->GetResourceHandler(browser, frame, request);
        }

        /////////////////////////////////////
        // render handler
        void GetViewRect(CefRefPtr<CefBrowser> browser, CefRect &rect) override
        {
            rect = CefRect(0, 0, _width, _height);
            _resize_done = true;
        }

        // Execute JavaScript Function at global scope (window)
        template <class... T>
        void ExecuteFunction(const string fcn_name, T &&...user_args)
        {
            if (_browser)
            {

                auto msg = CefProcessMessage::Create("fcn");
                auto args = msg->GetArgumentList();
                int arg_idx = 1;
                // Function name
                args->SetString(0, fcn_name);
                // Arguments
                for (auto &&user_arg : {user_args...})
                {
                    SetArgument(args, arg_idx, user_arg);
                    arg_idx++;
                }

                _browser->GetMainFrame()->SendProcessMessage(PID_RENDERER, msg);
            }
        }

        // Execute JavaScript Function at global scope (window)
        void ExecuteFunction(const string fcn_name)
        {
            if (_browser)
            {
                auto msg = CefProcessMessage::Create("fcn");
                auto args = msg->GetArgumentList();
                int arg_idx = 1;
                // Function name
                args->SetString(0, fcn_name);

                _browser->GetMainFrame()->SendProcessMessage(PID_RENDERER, msg);
            }
        }

        void
        OnPaint(CefRefPtr<CefBrowser> browser,
                PaintElementType type,
                const RectList &dirtyRects,
                const void *buffer,
                int width,
                int height) override
        {
            if (buffer && _on_paint_callback && type == PaintElementType::PET_VIEW)
            {
                _on_paint_callback(width, height, buffer);
            }
        }

        bool OnConsoleMessage(CefRefPtr<CefBrowser> browser,
                              cef_log_severity_t level,
                              const CefString &message,
                              const CefString &source,
                              int line) override
        {
            if (_logging)
            {
                cout << "[CEF] " << message << endl;
                return true;
            }
            else
            {
                return false;
            }
        }

    private:
        CefRefPtr<CefResourceManager> m_resourceManager;

        CefRefPtr<CefMessageRouterBrowserSide> m_messageRouter;
        scoped_ptr<CefMessageRouterBrowserSide::Handler> m_messageHandler;

        IMPLEMENT_REFCOUNTING(HeadlessClient);
        DISALLOW_COPY_AND_ASSIGN(HeadlessClient);
    }; // namespace CEF

} // namespace CEF
#endif