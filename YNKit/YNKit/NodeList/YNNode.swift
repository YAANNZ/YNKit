//
//  YNNode.swift
//  YNKit
//
//  Created by yanan.zhu on 2023/8/31.
//

import UIKit

class YNNode: NSObject {
    var value: Int = 0
    var next: YNNode?
    
    init(value: Int) {
        self.value = value
    }
}
