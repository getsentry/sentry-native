namespace sentry
{
namespace crashpad
{
int init(void);
int set_tag(const char *key, const char *value);
int set_release(const char *release);
} // namespace crashpad
} // namespace sentry
