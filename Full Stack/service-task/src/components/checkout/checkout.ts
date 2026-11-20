import { Component, inject, signal } from '@angular/core';
import { CurrencyPipe } from '@angular/common';
import { ShoppingCart } from '../../services/shopping-cart';

@Component({
  selector: 'app-checkout',
  imports: [CurrencyPipe],
  templateUrl: './checkout.html',
  styleUrl: './checkout.css'
})
export class Checkout {
  protected readonly cart = inject(ShoppingCart);

  protected readonly orderPlaced = signal(false);

  placeOrder(): void {
    this.cart.clear();
    this.orderPlaced.set(true);
  }
}