import { Injectable, computed, signal } from '@angular/core';
import { CartItem } from '../models/cart';
import { Product } from '../models/product';

@Injectable({
  providedIn: 'root'
})
export class ShoppingCart {
  private readonly items = signal<CartItem[]>([]);

  readonly cartItems = this.items.asReadonly();

  readonly total = computed(() =>
    this.items().reduce(
      (sum, item) => sum + item.product.price * item.quantity,
      0
    )
  );

  add(product: Product): void {
    const exists = this.items().some(item => item.product.id === product.id);

    if (exists) {
      this.increase(product.id);
    } else {
      this.items.update(items => [...items, { product, quantity: 1 }]);
    }
  }

  increase(productId: number): void {
    this.items.update(items =>
      items.map(item =>
        item.product.id === productId
          ? { ...item, quantity: item.quantity + 1 }
          : item
      )
    );
  }

  decrease(productId: number): void {
    this.items.update(items =>
      items.map(item =>
        item.product.id === productId && item.quantity > 1
          ? { ...item, quantity: item.quantity - 1 }
          : item
      )
    );
  }

  remove(productId: number): void {
    this.items.update(items =>
      items.filter(item => item.product.id !== productId)
    );
  }

  clear(): void {
    this.items.set([]);
  }
}