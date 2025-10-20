#pragma once
#include <vector>
#include <list>
#include <cassert>

namespace sakura {

	class ReferenceBase;
	class ObjectSystem;
	
	//GC対象オブジェクト
	class CollectableBase {
		friend class ObjectSystem;
		friend class ReferenceBase;
	private:
		//登録リスト管理
		CollectableBase* listNext;
		CollectableBase* listPrev;
		ObjectSystem* manager;
		bool isMarked;

		//参照カウンタ
		uint32_t referenceCount;

	private:
		ObjectSystem* GetManager() { return manager; }

	protected:
		CollectableBase() :
			listNext(nullptr),
			listPrev(nullptr),
			manager(nullptr),
			isMarked(false),
			referenceCount(0)
		{}

		virtual ~CollectableBase() {}

	public:

		//参照先の収集
		virtual void FetchReferencedItems(std::list<CollectableBase*>& result) = 0;
		
	};

	//参照基底。テンプレートに依存しない部分として。
	class ReferenceBase {
	private:
		CollectableBase* reference;
		ObjectSystem* manager;

		void IncrementReference();
		void DecrementReference();
	protected:
		ReferenceBase(CollectableBase* ref):
			reference(nullptr),
			manager(nullptr)
		{
			SetReference(ref);
		}

		void SetReference(CollectableBase* ref) {
			DecrementReference();
			if (ref != nullptr) {
				//マネージャもここでもつ。
				//スイープステップ中に、スイープ中か知る必要があるのにオブジェクトが既に解放されていてオブジェクト経由で参照できない状況をふせぐため。
				reference = ref;
				manager = ref->manager;
				IncrementReference();
			}
			else {
				reference = nullptr;
				manager = nullptr;
			}
		}

		CollectableBase* GetReference() const {
			return reference;
		}

		void ReleaseReference() {
			SetReference(nullptr);
		}
	};

	//参照型
	template<typename CollectableType>
	class Reference : protected ReferenceBase {
	private:
		CollectableType* GetReference() const { return static_cast<CollectableType*>(ReferenceBase::GetReference()); }

	public:
		Reference(CollectableType* obj): ReferenceBase(obj) {}

		Reference(const Reference<CollectableType>& ref):ReferenceBase(ref.GetReference()) {}

		Reference():ReferenceBase(nullptr){}

		~Reference() {
			ReleaseReference();
		}

		void operator= (const Reference<CollectableType>& ref) {

			SetReference(ref.GetReference());
		}

		//よくつかうインターフェース
		CollectableType* operator->() const{
			return static_cast<CollectableType*>(GetReference());
		}

		bool operator== (const CollectableType* v) const {
			return GetReference() == v;
		}

		bool operator!= (const CollectableType* v) const {
			return GetReference() != v;
		}

		CollectableType* Get() { return static_cast<CollectableType*>(GetReference()); }

		template<typename T>
		operator Reference<T>() const
		{
			return Reference<T>(GetReference());
		}

		template<typename T>
		Reference<T> Cast() const
		{
			return Reference<T>(static_cast<T*>(GetReference()));
		}

		CollectableType* Get() const {
			return GetReference();
		}
	};

	class ObjectSystem;

	//GCオブジェクトマネージャ
	//とても簡易的なマークアンドスイープとして実装
	class ObjectSystem {
		friend class CollectableBase;
		friend class ReferenceBase;
	private:
		bool isSweeping_;
		bool isDestructorExecuting_;
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

			item->manager = this;
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

			item->manager = nullptr;
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

		//リファレンスカウンタ加算
		void IncrementReference(CollectableBase* item) {

			if (isDestructorExecuting_) {
				//最終的な開放中はリファレンスカウンタは一切触らない
				return;
			}

			if (item != nullptr) {
				item->referenceCount++;
			}
		}

		//リファレンスカウンタ減算
		void DecrementReference(CollectableBase* item) {

			if (isDestructorExecuting_) {
				//最終的な開放中はリファレンスカウンタは一切触らない
				return;
			}

			if (item != nullptr) {
				assert(item->referenceCount > 0);
				if (!isSweeping_ && item->referenceCount == 1) {
					//スイープ中は開放しない。スイープ中に参照が0になるということはGCに開放されるはずなのでそのままでいい
					RemoveItem(item);
				}
				else if (item->referenceCount > 0) {
					item->referenceCount--;
				}
			}
		}

	public:
		ObjectSystem() :
			isSweeping_(false),
			isDestructorExecuting_(false),
			itemFirst(nullptr),
			itemLast(nullptr),
			itemCount(0)
		{}

		~ObjectSystem() {
			isSweeping_ = true;
			isDestructorExecuting_ = true;
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

			//スイープ中としてマーク
			assert(!isSweeping_);
			isSweeping_ = true;

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

			//スイープ中マークを解除
			isSweeping_ = false;
		}

	};
}
