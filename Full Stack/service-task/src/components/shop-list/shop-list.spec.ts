import { ComponentFixture, TestBed } from '@angular/core/testing';
import { ShopList } from './shop-list';

describe('ShopList', () => {
  let component: ShopList;
  let fixture: ComponentFixture<ShopList>;

  beforeEach(async () => {
    await TestBed.configureTestingModule({
      imports: [ShopList],
    }).compileComponents();

    fixture = TestBed.createComponent(ShopList);
    component = fixture.componentInstance;
    await fixture.whenStable();
  });

  it('should create', () => {
    expect(component).toBeTruthy();
  });
});
