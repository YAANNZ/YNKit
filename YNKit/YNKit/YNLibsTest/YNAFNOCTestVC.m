//
//  YNAFNOCTestVC.m
//  YNKit
//
//  Created by zhuyn on 2024/1/4.
//

#import "YNAFNOCTestVC.h"
#import <AFNetworking/AFNetworking.h>
#import "EventSource.h"

@interface YNAFNOCTestVC ()

@end

@implementation YNAFNOCTestVC

- (void)viewDidLoad {
    [super viewDidLoad];
    
    self.view.backgroundColor = [UIColor whiteColor];
    
    AFJSONRequestSerializer *requestSerializer = [AFJSONRequestSerializer serializer];
    NSMutableURLRequest *urlRequest = [requestSerializer requestWithMethod:@"POST" URLString:@"http://localhost:8080/crm2/customer/list" parameters:nil error:nil];
    urlRequest.timeoutInterval = 10;
    AFHTTPSessionManager *sessionManager = [AFHTTPSessionManager manager];
    
    NSURLSessionDataTask *dataTask = [sessionManager dataTaskWithRequest:urlRequest uploadProgress:^(NSProgress * _Nonnull uploadProgress) {
    } downloadProgress:^(NSProgress * _Nonnull downloadProgress) {
    } completionHandler:^(NSURLResponse * _Nonnull response, id  _Nullable responseObject, NSError * _Nullable error) {
        id jsonObject = responseObject;
        NSError *dataError = error;
        if ([responseObject isKindOfClass:[NSData class]]) {
            NSString *responseString = [[NSString alloc] initWithData:responseObject encoding:NSUTF8StringEncoding];
//            NSLog(@"MethodName:\n%@\n\nResponse:\n%@\n\n", aModel.requestUrl, responseString);
            
            AFJSONResponseSerializer *jsonResponseSerializer = [AFJSONResponseSerializer serializerWithReadingOptions:NSJSONReadingMutableContainers];
            jsonResponseSerializer.acceptableContentTypes = [NSSet setWithObjects:@"application/json", @"text/json", @"text/javascript", @"text/html", @"text/plain", nil];
            NSError *transformDataError = nil;
            jsonObject = [jsonResponseSerializer responseObjectForResponse:response data:responseObject error:&transformDataError];
            if (transformDataError != nil) {
                dataError = transformDataError;
            }
        }
        
    }];
    
    
    [dataTask resume];
    
}

- (void)eventSource {
    NSString *url = @"http://www.";
    url = [url stringByAddingPercentEncodingWithAllowedCharacters:[NSCharacterSet URLQueryAllowedCharacterSet]];
    
    EventSource *eventSource = [EventSource eventSourceWithURL:[NSURL URLWithString:url]];
//    self.eventSource = eventSource;
    [eventSource onMessage:^(Event *e) {
        dispatch_async(dispatch_get_main_queue(), ^{
            NSLog(@"%@", e);
            
        });
    }];
    
    [eventSource onSubject:^(Event *event) {
        dispatch_async(dispatch_get_main_queue(), ^{
//                    NSLog(@"%@", event);
            
        });
    }];
    
    [eventSource onEnd:^(Event *event) {
        dispatch_async(dispatch_get_main_queue(), ^{
//                    NSLog(@"%@", event); // 结束处理
            
        });
    }];

    [eventSource onError:^(Event *event) {
        dispatch_async(dispatch_get_main_queue(), ^{
//                    NSLog(@"%@", event);
//                    NSLog(@"error:%@", event.error);
            
        });
    }];
}

@end
