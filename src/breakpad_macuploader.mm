#if defined(SENTRY_BREAKPAD) && defined(__APPLE__)

#include <string>
#include <map>
#include <Foundation/NSString.h>
#import "common/mac/HTTPMultipartUpload.h"

namespace sentry {

int upload(std::string minidump_url,
           std::map<std::string, std::string> attachments) {
    @autoreleasepool {
        NSURL *url = [NSURL URLWithString:[NSString stringWithUTF8String:minidump_url.c_str()]];
        HTTPMultipartUpload *upload = [[HTTPMultipartUpload alloc] initWithURL:url];
        NSMutableDictionary *uploadParameters = [NSMutableDictionary dictionary];

        for (auto iter = attachments.cbegin(); iter != attachments.cend(); ++iter) {
            [upload addFileAtPath:[NSString stringWithUTF8String:iter->second.c_str()]
            name:[NSString stringWithUTF8String:iter->first.c_str()]];
        }

        NSError *error = nil;
        NSData *data = [upload send:&error];
        [data release];
        return error != nil;
    }
}

}

#endif
