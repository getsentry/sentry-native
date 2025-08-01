#include "sentry_boot.h"

#include <XNetworking.h>

static void
sentry__transport_network_connectivity_hint_changed_callback(_In_ void *context,
    _In_ const XNetworkingConnectivityHint *connectivity_hint)
{
    HANDLE network_initialized_event = static_cast<HANDLE>(context);
    if (connectivity_hint->networkInitialized) {
        (void)SetEvent(network_initialized_event);
    }
}

extern "C" {

HRESULT
sentry__transport_ensure_network_initialized()
{
    HRESULT hr = S_OK;
    XNetworkingConnectivityHint connectivity_hint;
    XTaskQueueHandle queue;

    hr = XTaskQueueCreate(XTaskQueueDispatchMode::Immediate,
        XTaskQueueDispatchMode::Immediate, &queue);
    if (SUCCEEDED(hr)) {
        // Use the new XNetworking APIs to check if the network is initialized.
        hr = XNetworkingGetConnectivityHint(&connectivity_hint);
        if (SUCCEEDED(hr)) {
            if (!connectivity_hint.networkInitialized) {
                // The network isn't initialized. Wait until the network becomes
                // initialized.
                HANDLE network_initialized_event
                    = CreateEvent(nullptr, TRUE, FALSE, nullptr);
                if (network_initialized_event != nullptr) {
                    XTaskQueueRegistrationToken token;
                    hr = XNetworkingRegisterConnectivityHintChanged(queue,
                        network_initialized_event,
                        sentry__transport_network_connectivity_hint_changed_callback,
                        &token);
                    if (SUCCEEDED(hr)) {
                        DWORD result = WaitForSingleObjectEx(
                            network_initialized_event, INFINITE, FALSE);
                        if (result != WAIT_OBJECT_0) {
                            hr = HRESULT_FROM_WIN32(GetLastError());
                        }

                        XNetworkingUnregisterConnectivityHintChanged(
                            token, true);
                    }

                    CloseHandle(network_initialized_event);
                } else {
                    hr = HRESULT_FROM_WIN32(GetLastError());
                }
            }
        }

        XTaskQueueCloseHandle(queue);
    }

    return hr;
}
}
