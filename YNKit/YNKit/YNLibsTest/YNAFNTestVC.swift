//
//  YNAFNTestVC.swift
//  YNKit
//
//  Created by Zhu Yanan（Lofty Team） on 2023/12/18.
//

import UIKit
import AFNetworking

class YNAFNTestVC: UIViewController {

    override func viewDidLoad() {
        super.viewDidLoad()

        self.view.backgroundColor = UIColor.white
        
        var requestSerializer = AFJSONRequestSerializer()
//        var requestSerializer = AFHTTPRequestSerializer()
        requestSerializer.timeoutInterval = 20;
//        requestSerializer.setValue(<#T##value: String?##String?#>, forHTTPHeaderField: <#T##String#>)
        requestSerializer.httpShouldHandleCookies = false
        
        do {
            var urlRequest = try requestSerializer.request(withMethod: "POST", urlString: "", parameters: {})


        } catch {

        }
        
        
        

        
        
        
        
//        NSMutableURLRequest *urlRequest = [requestSerializer requestWithMethod:(aModel.requestType==RRRequestTypePost?@"POST":@"GET") URLString:aModel.requestUrl parameters:aModel.paramDict error:nil];
//
//        RRRequestTask *requestTask = [RRRequestTask new];
//
//        requestTask.dataTask = [self.sessionManager dataTaskWithRequest:urlRequest uploadProgress:^(NSProgress * _Nonnull uploadProgress) {
//        } downloadProgress:^(NSProgress * _Nonnull downloadProgress) {
//        } completionHandler:^(NSURLResponse * _Nonnull response, id  _Nullable responseObject, NSError * _Nullable error) {
//            id jsonObject = responseObject;
//            NSError *dataError = error;
//            if ([responseObject isKindOfClass:[NSData class]]) {
//                NSString *responseString = [[NSString alloc] initWithData:responseObject encoding:NSUTF8StringEncoding];
//                NSLog(@"MethodName:\n%@\n\nResponse:\n%@\n\n", aModel.requestUrl, responseString);
//
//                AFJSONResponseSerializer *jsonResponseSerializer = [AFJSONResponseSerializer serializerWithReadingOptions:NSJSONReadingMutableContainers];
//                jsonResponseSerializer.acceptableContentTypes = [NSSet setWithObjects:@"application/json", @"text/json", @"text/javascript", @"text/html", @"text/plain", nil];
//                NSError *transformDataError = nil;
//                jsonObject = [jsonResponseSerializer responseObjectForResponse:response data:responseObject error:&transformDataError];
//                if (transformDataError != nil) {
//                    dataError = transformDataError;
//                }
//            }
//            if (aBlock) {
//                aBlock(jsonObject, jsonObject!=nil, dataError);
//            }
//        }];
//
//
//        [requestTask.dataTask resume];
//        return requestTask;

        
        
    }
    
    
    override func viewWillAppear(_ animated: Bool) {
        super.viewWillAppear(animated)
        
        let manager = AFHTTPSessionManager.init()
        
        let task = manager.get("http://localhost:8080/crm2/customer/list", parameters: [], headers: [:], progress: nil) { tast, anyObj in
            print(anyObj)
        }
        
        task?.resume()
    }

    

}
