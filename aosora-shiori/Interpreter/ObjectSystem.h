#pragma once
#include <vector>
#include <list>

namespace sakura {

	//GC対象オブジェクト
	class CollectableBase {
		friend class ObjectSystem;
	private:
		//登録リスト管理
		CollectableBase* listNext;
		CollectableBase* listPrev;
		bool isMarked;

	protected:
		CollectableBase() :
			listNext(nullptr),
			listPrev(nullptr),
			isMarked(false)
		{}

		virtual ~CollectableBase() {}

	public:

		//参照先の収集
		virtual void FetchReferencedItems(std::list<CollectableBase*>& result) = 0;
	};

	//参照型
	//TODO: 参照カウンタも導入して可能ならGCを待たずに捨ててもいいかも
	template<typename CollectableType>
	class Reference{
	private:
		CollectableType* reference;

	public:
		Reference(CollectableType* obj):
			reference(obj)
		{}

		Reference(const Reference<CollectableType>& ref):
			reference(ref.reference)
		{}

		Reference():reference(nullptr){}

		//よくつかうインターフェース
		CollectableType* operator->() const{
			return static_cast<CollectableType*>(reference);
		}

		bool operator== (const CollectableType* v) const {
			return reference == v;
		}

		bool operator!= (const CollectableType* v) const {
			return reference != v;
		}

		CollectableType* Get() { return static_cast<CollectableType*>(reference); }

		template<typename T>
		operator Reference<T>() const
		{
			return Reference<T>(reference);
		}

		template<typename T>
		Reference<T> Cast() const
		{
			return Reference<T>(static_cast<T*>(reference));
		}

		CollectableType* Get() const {
			return reference;
		}
	};

	class ObjectSystem;

	//GCオブジェクトマネージャ
	//とても簡易的なマークアンドスイープとして実装
	class ObjectSystem {
	private:
		CollectableBase* itemFirst;
		CollectableBase* itemLast;
		std::size_t itemCount;

	private:

		//アイテムの登録
		void AddItem(CollectableBase* item) {
			if (itemFirst == nullptr) {
				//最初のアイテム
				itemFirst = item;
				itemLast = item;
				item->listPrev = nullptr;
				item->listNext = nullptr;
			}
			else {
				//2つ目以降のアイテム
				item->listPrev = itemLast;
				item->listNext = nullptr;
				itemLast->listNext = item;
				itemLast = item;
			}

			itemCount++;
		}

		//アイテムの登録解除
		void RemoveItem(CollectableBase* item) {

			if (item->listNext != nullptr) {
				item->listNext->listPrev = item->listPrev;
			}

			if (item->listPrev != nullptr) {
				item->listPrev->listNext = item->listNext;
			}

			if (item->listNext == nullptr) {
				//自分のnextがnullならlastが書き換わる
				itemLast = item->listPrev;
			}
			if (item->listPrev == nullptr) {
				//first側も同じ
				itemFirst = item->listNext;
			}

			delete item;
			itemCount--;
		}

		//アイテムのマーキング
		void MarkReferences(CollectableBase* item) {

			std::list<CollectableBase*> children;
			item->FetchReferencedItems(children);

			for (CollectableBase* c : children) {
				if (c == nullptr) {
					continue;
				}

				if (!c->isMarked) {
					c->isMarked = true;
					MarkReferences(c);
				}
			}
		}

	public:
		ObjectSystem() :
			itemFirst(nullptr),
			itemLast(nullptr),
			itemCount(0)
		{}

		~ObjectSystem() {
			CollectableBase* item = itemFirst;
			while (item != nullptr) {
				CollectableBase* next = item->listNext;
				delete item;
				item = next;
			}
		}

		//オブジェクト作成
		template<typename CollectableType, typename... Args>
		Reference<CollectableType> CreateObject(Args... args) {
			CollectableBase* item = new CollectableType(args...);
			AddItem(item);
			return Reference<CollectableType>(static_cast<CollectableType*>(item));
		}

		//オブジェクト回収
		//引数で渡すコレクションから辿れるすべての参照をマークし、マークされなかったものを破棄
		void CollectObjects(const std::vector<CollectableBase*> rootObjects) {

			//状態をリセット
			CollectableBase* current = itemFirst;
			while (current != nullptr) {
				current->isMarked = false;
				current = current->listNext;
			}

			//参照のマーキング
			for (CollectableBase* item : rootObjects) {
				item->isMarked = true;
				MarkReferences(item);
			}

			//無参照リストを作成
			std::list<CollectableBase*> removeItems;
			current = itemFirst;
			if (current != nullptr) {
				if (!current->isMarked) {
					//マークのついてないオブジェクトは参照されてないので削除する
					removeItems.push_back(current);
				}
				current = current->listNext;
			}

			//無参照のアイテムを削除
			for (CollectableBase* item : removeItems) {
				RemoveItem(item);
			}
		}

	};
}
