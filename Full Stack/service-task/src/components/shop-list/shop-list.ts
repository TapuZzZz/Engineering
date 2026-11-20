import { Component, inject } from '@angular/core';
import { CurrencyPipe } from '@angular/common';
import { Product } from '../../models/product';
import { ShoppingCart } from '../../services/shopping-cart';

@Component({
  selector: 'app-shop-list',
  imports: [CurrencyPipe],
  templateUrl: './shop-list.html',
  styleUrl: './shop-list.css'
})
export class ShopList {
  protected readonly cart = inject(ShoppingCart);

  protected readonly products: Product[] = [
    { id: 1, name: 'Apple', price: 3.5 },
    { id: 2, name: 'Bread', price: 8 },
    { id: 3, name: 'Milk', price: 6.9 },
    { id: 4, name: 'Cheese', price: 14.9 },
    { id: 5, name: 'Eggs', price: 12 },
    { id: 6, name: 'Banana', price: 5.5 }
  ];

  addToCart(product: Product): void {
    this.cart.add(product);
  }
}