//
//  YNNodeListTestVC.swift
//  YNKit
//
//  Created by yanan.zhu on 2023/8/31.
//

import UIKit

class YNNodeListTestVC: UIViewController {

    var nodeList: YNNodeList?
    
    override func viewDidLoad() {
        super.viewDidLoad()

        var headNode = YNNode(value: 1)
        nodeList = YNNodeList.init(head: headNode)
        
        
    }
    
    
}
