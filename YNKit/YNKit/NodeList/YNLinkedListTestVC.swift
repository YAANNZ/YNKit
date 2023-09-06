//
//  YNLinkedListTestVC.swift
//  YNKit
//
//  Created by yanan.zhu on 2023/9/6.
//

import UIKit

class YNLinkedListTestVC: UIViewController {

    @IBOutlet weak var contentLabel: UILabel!
    var nodeList: YNLinkedList!
    
    override func viewDidLoad() {
        super.viewDidLoad()

        let endNode = YNNode(value: "3")
        let midNode = YNNode(value: "2")
        let headNode = YNNode(value: "1")
        headNode.next = midNode;
        midNode.next = endNode;
        nodeList = YNLinkedList.init(head: headNode)
        
        showNodeValues(nodeList)
    }
    
    func showNodeValues(_ nodeList: YNLinkedList) {
        let textStr = (nodeList.head.value ?? "") + "-" + (nodeList.head.next?.value ?? "")
        contentLabel.text = textStr  + "-" + (nodeList.head.next?.next?.value ?? "")
    }
    
    @IBAction func invertEvent(_ sender: Any) {
        // 迭代反转链表
        var newHeadNode: YNNode?
        var headNode = nodeList.head
        
        while headNode != nil {
            let tempNode = headNode
            headNode = headNode?.next
            tempNode?.next = newHeadNode
            newHeadNode = tempNode
        }
        
        nodeList.head = newHeadNode
        
        // 递归反转链表
        
        
        showNodeValues(nodeList)
    }
    
}

func invertHeadNode(_ headNode: YNNode?) -> YNNode? {
    if headNode == nil || headNode?.next == nil {
        return headNode
    }
    
    let newHead = invertHeadNode(headNode?.next)
    headNode?.next?.next = headNode
    headNode?.next = nil
    return newHead
}
