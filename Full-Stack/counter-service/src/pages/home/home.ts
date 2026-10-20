import { Component, inject } from '@angular/core';
import { Counter } from '../../services/counter';

@Component({
  imports: [],
  selector: 'app-home',
  styleUrl: './home.css',
  templateUrl: './home.html',
})
export class Home {
  counterService: Counter = inject(Counter);
}
