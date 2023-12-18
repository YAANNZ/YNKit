//
//  YNNode.swift
//  YNKit
//
//  Created by yanan.zhu on 2023/8/31.
//

import UIKit

class YNNode: NSObject {
    var value: String?
    var next: YNNode?
    
    init(value: String) {
        self.value = value
    }
}
