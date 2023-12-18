//
//  YNLinkedListTestVC.swift
//  YNKit
//
//  Created by yanan.zhu on 2023/9/6.
//

/*
 1. 反转链表
 2. 判断两个链表有交集
 */

import UIKit

class YNLinkedListTestVC: UIViewController {

    @IBOutlet weak var contentLabel: UILabel!
    var nodeList: YNLinkedList!
    
    override func viewDidLoad() {
        super.viewDidLoad()

        
        nodeList = createLinkedList(["1", "2", "3", "4", "5"])
        
        showNodeValues(nodeList)
    }
    
    
    
    @IBAction func invertEvent(_ sender: Any) {
        // 迭代反转链表
        nodeList.head = invertHeadNodeIterator(nodeList.head)
        
        // 递归反转链表
//        nodeList.head = invertHeadNode(nodeList.head)
        
        showNodeValues(nodeList)
    }
    
    func showNodeValues(_ nodeList: YNLinkedList) {
        var headNode = nodeList.head
        
        let textAry: NSMutableArray! = NSMutableArray()
        while headNode != nil {
            textAry.add(headNode?.value as Any)
            headNode = headNode?.next
        }
        
        contentLabel.text = textAry.componentsJoined(by: "-")
    }
    
}

//MARK: - Private Tool
func createLinkedList(_ arr: Array<String>) -> YNLinkedList? {
    
    var headNode: YNNode!
    for i in (0..<arr.count).reversed() {
        let node = YNNode(value: arr[i])
        node.next = headNode
        headNode = node
    }
    
    return YNLinkedList(head: headNode)
}

//MARK: - 反转链表
func invertHeadNodeIterator(_ headNode: YNNode?) -> YNNode? {
    
    var newHeadNode: YNNode?
    var headNode = headNode

    while headNode != nil {
        let tempNode = headNode
        headNode = headNode?.next
        tempNode?.next = newHeadNode
        newHeadNode = tempNode
    }

    return newHeadNode
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
