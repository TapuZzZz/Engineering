import { Component } from '@angular/core';
import { CurrencyPipe } from '@angular/common';

interface Pizza {
  id: number;
  name: string;
  description: string;
  price: number;
  emoji: string;
  vegetarian: boolean;
}

@Component({
  selector: 'app-menu',
  imports: [CurrencyPipe],
  templateUrl: './menu.html',
  styleUrl: './menu.css'
})
export class Menu {
  pizzas: Pizza[] = [
    { id: 1, name: 'Margherita', description: 'Tomato sauce, mozzarella, fresh basil', price: 48, emoji: '🍕', vegetarian: true },
    { id: 2, name: 'Pepperoni', description: 'Tomato sauce, mozzarella, spicy pepperoni', price: 58, emoji: '🌶️', vegetarian: false },
    { id: 3, name: 'Mushrooms', description: 'Mozzarella, mushrooms, garlic, thyme', price: 54, emoji: '🍄', vegetarian: true },
    { id: 4, name: 'Greek', description: 'Feta, olives, red onion, tomatoes', price: 56, emoji: '🫒', vegetarian: true },
    { id: 5, name: 'Tuna', description: 'Tuna, corn, onion, mozzarella', price: 58, emoji: '🐟', vegetarian: false },
    { id: 6, name: 'Four Cheese', description: 'Mozzarella, parmesan, gorgonzola, ricotta', price: 62, emoji: '🧀', vegetarian: true }
  ];
}